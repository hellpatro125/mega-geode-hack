#include "FramePrediction.hpp"
#include "../Config.hpp"

using namespace geode::prelude;

bool FramePredictionManager::willCollideWithHazard(PlayerObject* player, PlayLayer* playLayer, int framesAhead) {
    if (!player || !playLayer || !playLayer->m_objects) return false;

    cocos2d::CCPoint playerPos = player->getPosition();
    float xVel = 311.58f; // Standard 1x speed in Geometry Dash
    float yVel = 12.0f;   // Estimated jump impulse
    float gravity = -0.9f;

    cocos2d::CCRect playerBox = player->getObjectRect();
    float boxW = playerBox.size.width;
    float boxH = playerBox.size.height;

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

        auto* objects = playLayer->m_objects;
        for (int i = 0; i < objects->count(); ++i) {
            auto* obj = static_cast<GameObject*>(objects->objectAtIndex(i));
            if (!obj || !obj->m_isActive || obj->m_hasBeenActivated) continue;

            cocos2d::CCPoint objPos = obj->getPosition();
            if (std::abs(objPos.x - simPos.x) > 150.0f) continue;

            // Check if hazard/spike
            if (obj->m_objectType == GameObjectType::Hazard) {
                cocos2d::CCRect hazardRect = obj->getObjectRect();
                if (simRect.intersectsRect(hazardRect)) {
                    return true;
                }
            }
        }
    }

    return false;
}

GameObject* FramePredictionManager::findApproachingOrb(PlayerObject* player, PlayLayer* playLayer, int windowFrames) {
    if (!player || !playLayer || !playLayer->m_objects) return nullptr;

    cocos2d::CCPoint playerPos = player->getPosition();
    float xVel = 311.58f;
    float stepDt = 1.0f / 240.0f;
    float maxDistanceAhead = xVel * stepDt * static_cast<float>(windowFrames);

    auto* objects = playLayer->m_objects;
    for (int i = 0; i < objects->count(); ++i) {
        auto* obj = static_cast<GameObject*>(objects->objectAtIndex(i));
        if (!obj || !obj->m_isActive || obj->m_hasBeenActivated) continue;

        // Check if interactive ring/orb
        if (obj->m_isRing || obj->m_objectType == GameObjectType::YellowJumpRing) {
            cocos2d::CCPoint ringPos = obj->getPosition();
            float dx = ringPos.x - playerPos.x;
            float dy = std::abs(ringPos.y - playerPos.y);

            if (dx > 0.0f && dx <= maxDistanceAhead && dy <= 60.0f) {
                return obj;
            }
        }
    }

    return nullptr;
}

bool FramePredictionManager::isCurrentFrameSafe(PlayerObject* player, PlayLayer* playLayer) {
    if (!player || !playLayer) return false;

    if (pendingClick.needsOrbActivation && pendingClick.targetOrb) {
        cocos2d::CCRect playerRect = player->getObjectRect();
        cocos2d::CCRect orbRect = pendingClick.targetOrb->getObjectRect();

        orbRect.origin.x -= 10.0f;
        orbRect.origin.y -= 10.0f;
        orbRect.size.width += 20.0f;
        orbRect.size.height += 20.0f;

        if (playerRect.intersectsRect(orbRect)) {
            return true;
        }
        return false;
    }

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
        lastDelayFrames = pendingClick.elapsedFrames;
        totalCorrections++;
        feedbackTimer = 1.0f;

        playLayer->handleButton(true, pendingClick.button, pendingClick.isPlayer1);
        pendingClick.active = false;
    } else if (pendingClick.elapsedFrames >= pendingClick.maxWaitFrames) {
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

            if (player && !player->m_isDead) {
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
                    return;
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
                        return;
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
