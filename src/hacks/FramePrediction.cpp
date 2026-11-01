#include "FramePrediction.hpp"
#include "../Config.hpp"

using namespace geode::prelude;

bool FramePredictionManager::willCollideWithHazard(PlayerObject* player, PlayLayer* playLayer, int framesAhead) {
    if (!player || !playLayer || !playLayer->m_collisionHash) return false;

    cocos2d::CCPoint playerPos = player->getPosition();
    float xVel = player->m_xVelocity;
    // Estimated vertical velocity if jumped:
    float yVel = player->m_jumpAccel;
    float gravity = player->m_gravity;

    // Simulate bounding box trajectory over `framesAhead` frames
    cocos2d::CCRect playerBox = player->getObjectRect();
    float boxW = playerBox.size.width;
    float boxH = playerBox.size.height;

    // Fixed step time in GD (240hz physics = 1/240.0f)
    float stepDt = 1.0f / 240.0f;

    cocos2d::CCPoint simPos = playerPos;
    float simYVel = yVel;

    for (int f = 1; f <= framesAhead; ++f) {
        simPos.x += xVel * stepDt;
        simPos.y += simYVel * stepDt;
        simYVel += gravity * stepDt;

        cocos2d::CCRect simRect(
            simPos.x - boxW * 0.5f,
            simPos.y - boxH * 0.5f,
            boxW,
            boxH
        );

        // Check against nearby hazard objects
        if (playLayer->m_hazardObjects) {
            auto* hazards = playLayer->m_hazardObjects;
            for (int i = 0; i < hazards->count(); ++i) {
                auto* obj = static_cast<GameObject*>(hazards->objectAtIndex(i));
                if (!obj || !obj->m_isActive || obj->m_hasBeenActivated) continue;

                cocos2d::CCRect hazardRect = obj->getObjectRect();
                // If within close range and intersecting
                if (simRect.intersectsRect(hazardRect)) {
                    return true; // Immediate jump leads to hazard collision!
                }
            }
        }
    }

    return false;
}

GameObject* FramePredictionManager::findApproachingOrb(PlayerObject* player, PlayLayer* playLayer, int windowFrames) {
    if (!player || !playLayer || !playLayer->m_ringObjects) return nullptr;

    cocos2d::CCPoint playerPos = player->getPosition();
    float xVel = player->m_xVelocity;
    if (xVel <= 0.0f) xVel = 300.0f; // fallback forward velocity
    float stepDt = 1.0f / 240.0f;

    float maxDistanceAhead = xVel * stepDt * static_cast<float>(windowFrames);

    auto* rings = playLayer->m_ringObjects;
    for (int i = 0; i < rings->count(); ++i) {
        auto* ring = static_cast<GameObject*>(rings->objectAtIndex(i));
        if (!ring || !ring->m_isActive || ring->m_hasBeenActivated) continue;

        cocos2d::CCPoint ringPos = ring->getPosition();
        float dx = ringPos.x - playerPos.x;
        float dy = std::abs(ringPos.y - playerPos.y);

        // If ring is ahead of player within the prediction window distance
        if (dx > 0.0f && dx <= maxDistanceAhead && dy <= 60.0f) {
            return ring;
        }
    }

    return nullptr;
}

bool FramePredictionManager::isCurrentFrameSafe(PlayerObject* player, PlayLayer* playLayer) {
    if (!player || !playLayer) return false;

    // Check if player is currently in valid jump state or touching an orb
    if (pendingClick.needsOrbActivation && pendingClick.targetOrb) {
        cocos2d::CCRect playerRect = player->getObjectRect();
        cocos2d::CCRect orbRect = pendingClick.targetOrb->getObjectRect();

        // Expanding orb activation hitbox slightly for forgiveness
        orbRect.origin.x -= 10.0f;
        orbRect.origin.y -= 10.0f;
        orbRect.size.width += 20.0f;
        orbRect.size.height += 20.0f;

        if (playerRect.intersectsRect(orbRect)) {
            return true;
        }
        return false;
    }

    // Check hazard clearance for current frame jump
    return !willCollideWithHazard(player, playLayer, 3);
}

void FramePredictionManager::step(PlayLayer* playLayer, float dt) {
    if (feedbackTimer > 0.0f) {
        feedbackTimer -= dt;
    }

    if (!pendingClick.active || !playLayer) return;

    auto* player = pendingClick.isPlayer1 ? playLayer->m_player1 : playLayer->m_player2;
    if (!player || player->m_isDead) {
        pendingClick.active = false;
        return;
    }

    pendingClick.elapsedFrames++;

    bool safeNow = isCurrentFrameSafe(player, playLayer);

    if (safeNow) {
        // Execute the delayed click on the frame where player won't die!
        lastDelayFrames = pendingClick.elapsedFrames;
        totalCorrections++;
        feedbackTimer = 1.0f;

        // Perform genuine button push
        playLayer->handleButton(true, pendingClick.button, pendingClick.isPlayer1);
        pendingClick.active = false;
    } else if (pendingClick.elapsedFrames >= pendingClick.maxWaitFrames) {
        // Window expired. If still unsafe, do not cause instant death, drop click or fire
        pendingClick.active = false;
    }
}

// ------------------- GEODE HOOKS -------------------

class $modify(PredictiveGameLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        auto& cfg = Config::get();
        auto* playLayer = typeinfo_cast<PlayLayer*>(this);

        if (cfg.predictionEnabled && playLayer && down && button == 1) {
            auto* player = isPlayer1 ? playLayer->m_player1 : playLayer->m_player2;

            if (player && !player->m_isDead && !player->m_hasCompletedLevel) {
                auto& mgr = FramePredictionManager::get();

                // Check 1: If approaching an orb in mid-air
                GameObject* upcomingOrb = nullptr;
                if (cfg.predictionMode != "Hazard Avoidance Only") {
                    upcomingOrb = mgr.findApproachingOrb(player, playLayer, cfg.predictionWindow);
                }

                if (upcomingOrb) {
                    mgr.pendingClick.active = true;
                    mgr.pendingClick.button = button;
                    mgr.pendingClick.isPlayer1 = isPlayer1;
                    mgr.pendingClick.maxWaitFrames = cfg.predictionWindow;
                    mgr.pendingClick.elapsedFrames = 0;
                    mgr.pendingClick.needsOrbActivation = true;
                    mgr.pendingClick.targetOrb = upcomingOrb;
                    return; // Intercepted: wait until player arrives at safe orb hitbox!
                }

                // Check 2: If jumping immediately hits a hazard within short lookahead
                if (cfg.predictionMode != "Orb Snapping Only") {
                    bool lethalJump = mgr.willCollideWithHazard(player, playLayer, 3);
                    if (lethalJump) {
                        mgr.pendingClick.active = true;
                        mgr.pendingClick.button = button;
                        mgr.pendingClick.isPlayer1 = isPlayer1;
                        mgr.pendingClick.maxWaitFrames = cfg.predictionWindow;
                        mgr.pendingClick.elapsedFrames = 0;
                        mgr.pendingClick.needsOrbActivation = false;
                        mgr.pendingClick.targetOrb = nullptr;
                        return; // Intercepted: delay jump until frame where player clears hazard!
                    }
                }
            }
        }

        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }
};

class $modify(PredictivePlayLayer, PlayLayer) {
    void update(float dt) {
        PlayLayer::update(dt);
        FramePredictionManager::get().step(this, dt);
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        FramePredictionManager::get().reset();
    }
};
