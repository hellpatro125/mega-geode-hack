#include "FramePrediction.hpp"
#include "../Config.hpp"

using namespace geode::prelude;

bool FramePredictionManager::willCollideWithHazard(PlayerObject* player, PlayLayer* playLayer, int framesAhead) {
    if (!player || !playLayer || !playLayer->m_objects) return false;

    cocos2d::CCPoint playerPos = player->getPosition();
    float speedMult = player->m_speedMultiplier > 0.0 ? static_cast<float>(player->m_speedMultiplier) : 0.9f;
    float xVel = (speedMult / 0.9f) * 311.58f;
    float gravityDir = player->m_isUpsideDown ? -1.0f : 1.0f;
    float dt = 1.0f / 240.0f;

    // Calculate vertical velocity upon clicking for specific gamemode
    float simYVel = 0.0f;
    float gravityAcc = -35.0f * gravityDir;

    if (player->m_isDart) {
        // Wave: moves diagonally up (+X, +Y)
        simYVel = xVel * gravityDir;
        gravityAcc = 0.0f;
    } else if (player->m_isShip) {
        // Ship: upward thrust
        simYVel = static_cast<float>(player->m_yVelocity) + (18.0f * gravityDir);
        gravityAcc = -18.0f * gravityDir;
    } else if (player->m_isBird) {
        // UFO: jump impulse
        simYVel = 10.5f * gravityDir;
        gravityAcc = -25.0f * gravityDir;
    } else if (player->m_isBall) {
        // Ball: inverted gravity
        simYVel = -static_cast<float>(player->m_yVelocity);
        gravityAcc = 35.0f * gravityDir;
    } else {
        // Cube / Robot
        simYVel = 12.0f * gravityDir;
        gravityAcc = -35.0f * gravityDir;
    }

    cocos2d::CCRect playerBox = player->getObjectRect();
    float boxW = playerBox.size.width;
    float boxH = playerBox.size.height;

    cocos2d::CCPoint simPos = playerPos;

    for (int f = 1; f <= framesAhead; ++f) {
        simPos.x += xVel * dt;
        simPos.y += simYVel * dt;
        simYVel += gravityAcc * dt;

        cocos2d::CCRect simRect(
            simPos.x - boxW * 0.5f,
            simPos.y - boxH * 0.5f,
            boxW,
            boxH
        );

        auto* objects = playLayer->m_objects;
        for (int i = 0; i < objects->count(); ++i) {
            auto* obj = static_cast<GameObject*>(objects->objectAtIndex(i));
            if (!obj || obj->m_isActivated) continue;

            cocos2d::CCPoint objPos = obj->getPosition();
            if (std::abs(objPos.x - simPos.x) > 120.0f) continue;

            if (obj->m_objectType == GameObjectType::Hazard) {
                if (simRect.intersectsRect(obj->getObjectRect())) {
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
    float speedMult = player->m_speedMultiplier > 0.0 ? static_cast<float>(player->m_speedMultiplier) : 0.9f;
    float xVel = (speedMult / 0.9f) * 311.58f;
    float dt = 1.0f / 240.0f;
    float maxDistanceAhead = xVel * dt * static_cast<float>(windowFrames);

    auto* objects = playLayer->m_objects;
    for (int i = 0; i < objects->count(); ++i) {
        auto* obj = static_cast<GameObject*>(objects->objectAtIndex(i));
        if (!obj || obj->m_isActivated) continue;

        // Check all interactive ring/orb types
        bool isRing = (obj->m_objectType == GameObjectType::YellowJumpRing ||
                       obj->m_objectType == GameObjectType::PinkJumpRing ||
                       obj->m_objectType == GameObjectType::GravityRing ||
                       obj->m_objectType == GameObjectType::RedJumpRing ||
                       obj->m_objectType == GameObjectType::GreenRing ||
                       obj->m_objectType == GameObjectType::DashRing);

        if (isRing) {
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

    // Check if landing condition met (for Cube / Ball / Robot / Spider)
    if (pendingClick.needsLanding) {
        if (player->m_isOnGround) {
            return true;
        }
    }

    // Check if orb activation reached
    if (pendingClick.needsOrbActivation && pendingClick.targetOrb) {
        cocos2d::CCRect playerRect = player->getObjectRect();
        cocos2d::CCRect orbRect = pendingClick.targetOrb->getObjectRect();

        orbRect.origin.x -= 12.0f;
        orbRect.origin.y -= 12.0f;
        orbRect.size.width += 24.0f;
        orbRect.size.height += 24.0f;

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

                // Check 1: If approaching any orb (all modes)
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
                    mgr.pendingClick.needsLanding = false;
                    mgr.pendingClick.targetOrb = upcomingOrb;
                    return;
                }

                // Check 2: Mid-air click before landing (Cube, Ball, Robot, Spider)
                bool isGroundMode = !player->m_isShip && !player->m_isDart && !player->m_isBird && !player->m_isSwing;
                if (isGroundMode && !player->m_isOnGround) {
                    mgr.pendingClick.active = true;
                    mgr.pendingClick.button = button;
                    mgr.pendingClick.isPlayer1 = isPlayer1;
                    mgr.pendingClick.maxWaitFrames = cfg.predictionWindow;
                    mgr.pendingClick.elapsedFrames = 0;
                    mgr.pendingClick.needsOrbActivation = false;
                    mgr.pendingClick.needsLanding = true;
                    mgr.pendingClick.targetOrb = nullptr;
                    return; // Wait until player touches down!
                }

                // Check 3: If clicking now collides with hazard (Wave, Ship, UFO, Cube)
                if (cfg.predictionMode != "Orb Snapping Only") {
                    bool lethalJump = mgr.willCollideWithHazard(player, playLayer, 3);
                    if (lethalJump) {
                        mgr.pendingClick.active = true;
                        mgr.pendingClick.button = button;
                        mgr.pendingClick.isPlayer1 = isPlayer1;
                        mgr.pendingClick.maxWaitFrames = cfg.predictionWindow;
                        mgr.pendingClick.elapsedFrames = 0;
                        mgr.pendingClick.needsOrbActivation = false;
                        mgr.pendingClick.needsLanding = false;
                        mgr.pendingClick.targetOrb = nullptr;
                        return; // Delay until path clears!
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
