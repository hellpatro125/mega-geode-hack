#include "FramePrediction.hpp"
#include "../Config.hpp"

using namespace geode::prelude;

static bool s_isDispatchingPredictedClick = false;

bool FramePredictionManager::willCollideWithHazard(PlayerObject* player, PlayLayer* playLayer, int framesAhead) {
    if (!player || !playLayer || !playLayer->m_objects) return false;

    CCPoint playerPos = player->getPosition();
    float speedMult = player->m_speedMultiplier > 0.0 ? static_cast<float>(player->m_speedMultiplier) : 0.9f;
    float xVel = (speedMult / 0.9f) * 311.58f;
    float gravityDir = player->m_isUpsideDown ? -1.0f : 1.0f;
    float dt = 1.0f / 240.0f;

    float simYVel = 0.0f;
    float gravityAcc = -35.0f * gravityDir;

    if (player->m_isDart) {
        simYVel = xVel * gravityDir;
        gravityAcc = 0.0f;
    } else if (player->m_isShip) {
        simYVel = static_cast<float>(player->m_yVelocity) + (18.0f * gravityDir);
        gravityAcc = -18.0f * gravityDir;
    } else if (player->m_isBird) {
        simYVel = 10.5f * gravityDir;
        gravityAcc = -25.0f * gravityDir;
    } else if (player->m_isBall) {
        simYVel = -static_cast<float>(player->m_yVelocity);
        gravityAcc = 35.0f * gravityDir;
    } else {
        simYVel = 12.0f * gravityDir;
        gravityAcc = -35.0f * gravityDir;
    }

    CCRect playerBox = player->getObjectRect();
    float boxW = std::max(16.0f, playerBox.size.width * 0.7f);
    float boxH = std::max(16.0f, playerBox.size.height * 0.7f);

    CCPoint simPos = playerPos;

    for (int f = 1; f <= framesAhead; ++f) {
        simPos.x += xVel * dt;
        simPos.y += simYVel * dt;
        simYVel += gravityAcc * dt;

        CCRect simRect(
            simPos.x - boxW * 0.5f,
            simPos.y - boxH * 0.5f,
            boxW,
            boxH
        );

        auto* objects = playLayer->m_objects;
        for (int i = 0; i < objects->count(); ++i) {
            auto* obj = static_cast<GameObject*>(objects->objectAtIndex(i));
            if (!obj || obj->m_isActivated) continue;

            CCPoint objPos = obj->getPosition();
            if (std::abs(objPos.x - simPos.x) > 60.0f) continue;

            if (obj->m_objectType == GameObjectType::Hazard || obj->m_objectType == GameObjectType::AnimatedHazard) {
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

    CCPoint playerPos = player->getPosition();
    float speedMult = player->m_speedMultiplier > 0.0 ? static_cast<float>(player->m_speedMultiplier) : 0.9f;
    float xVel = (speedMult / 0.9f) * 311.58f;
    float dt = 1.0f / 240.0f;
    float maxDistanceAhead = xVel * dt * static_cast<float>(windowFrames) + 10.0f;

    auto* objects = playLayer->m_objects;
    for (int i = 0; i < objects->count(); ++i) {
        auto* obj = static_cast<GameObject*>(objects->objectAtIndex(i));
        if (!obj || obj->m_isActivated) continue;

        bool isRing = (obj->m_objectType == GameObjectType::YellowJumpRing ||
                       obj->m_objectType == GameObjectType::PinkJumpRing ||
                       obj->m_objectType == GameObjectType::GravityRing ||
                       obj->m_objectType == GameObjectType::RedJumpRing ||
                       obj->m_objectType == GameObjectType::GreenRing ||
                       obj->m_objectType == GameObjectType::DashRing);

        if (isRing) {
            CCPoint ringPos = obj->getPosition();
            float dx = ringPos.x - playerPos.x;
            float dy = std::abs(ringPos.y - playerPos.y);

            if (dx > 0.0f && dx <= maxDistanceAhead && dy <= 70.0f) {
                return obj;
            }
        }
    }

    return nullptr;
}

bool FramePredictionManager::isCurrentFrameSafe(PlayerObject* player, PlayLayer* playLayer) {
    if (!player || !playLayer) return true;

    // Check if landing condition met (for Cube / Ball / Robot / Spider)
    if (pendingClick.needsLanding) {
        if (player->m_isOnGround) {
            return true;
        }
    }

    // Check if orb activation reached
    if (pendingClick.needsOrbActivation && pendingClick.targetOrb) {
        CCRect playerRect = player->getObjectRect();
        CCRect orbRect = pendingClick.targetOrb->getObjectRect();

        orbRect.origin.x -= 16.0f;
        orbRect.origin.y -= 16.0f;
        orbRect.size.width += 32.0f;
        orbRect.size.height += 32.0f;

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

    if (safeNow || pendingClick.elapsedFrames >= pendingClick.maxWaitFrames) {
        lastDelayFrames = pendingClick.elapsedFrames;
        if (safeNow && pendingClick.elapsedFrames > 0) {
            totalCorrections++;
            feedbackTimer = 1.0f;
        }

        s_isDispatchingPredictedClick = true;
        playLayer->handleButton(true, pendingClick.button, pendingClick.isPlayer1);
        s_isDispatchingPredictedClick = false;

        pendingClick.active = false;
    }
}

// ------------------- GEODE HOOKS -------------------

class $modify(PredictiveGameLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        // Re-entrancy guard: if we are delivering a predicted click, pass straight through!
        if (s_isDispatchingPredictedClick) {
            GJBaseGameLayer::handleButton(down, button, isPlayer1);
            return;
        }

        auto& cfg = Config::get();
        auto* playLayer = typeinfo_cast<PlayLayer*>(this);

        if (cfg.predictionEnabled && playLayer && button == 1) {
            auto& mgr = FramePredictionManager::get();

            // When user releases the button, flush any pending click so state stays synchronized!
            if (!down) {
                if (mgr.pendingClick.active && mgr.pendingClick.button == button) {
                    s_isDispatchingPredictedClick = true;
                    playLayer->handleButton(true, button, isPlayer1);
                    GJBaseGameLayer::handleButton(false, button, isPlayer1);
                    s_isDispatchingPredictedClick = false;
                    mgr.pendingClick.active = false;
                    return;
                }
                GJBaseGameLayer::handleButton(down, button, isPlayer1);
                return;
            }

            // Button down:
            auto* player = isPlayer1 ? playLayer->m_player1 : playLayer->m_player2;

            if (player && !player->m_isDead) {
                // If player is on ground and not in midair, normal ground clicks should NOT be intercepted!
                // Only intercept mid-air or lethal cases:

                // Check 1: If approaching any orb (all modes)
                if (cfg.predictionMode != "Hazard Avoidance Only") {
                    GameObject* upcomingOrb = mgr.findApproachingOrb(player, playLayer, cfg.predictionWindow);
                    if (upcomingOrb) {
                        CCRect playerRect = player->getObjectRect();
                        CCRect orbRect = upcomingOrb->getObjectRect();
                        // If already inside orb hitbox, execute immediately!
                        if (!playerRect.intersectsRect(orbRect)) {
                            mgr.pendingClick.active = true;
                            mgr.pendingClick.button = button;
                            mgr.pendingClick.isPlayer1 = isPlayer1;
                            mgr.pendingClick.maxWaitFrames = cfg.predictionWindow;
                            mgr.pendingClick.elapsedFrames = 0;
                            mgr.pendingClick.needsOrbActivation = true;
                            mgr.pendingClick.needsLanding = false;
                            mgr.pendingClick.targetOrb = upcomingOrb;
                            return; // Delay click until touching orb!
                        }
                    }
                }

                // Check 2: Mid-air click right before landing on ground (Cube, Ball, Robot, Spider)
                bool isGroundMode = !player->m_isShip && !player->m_isDart && !player->m_isBird && !player->m_isSwing;
                if (isGroundMode && !player->m_isOnGround && player->m_yVelocity <= 0.0) {
                    // Only delay if close to landing
                    mgr.pendingClick.active = true;
                    mgr.pendingClick.button = button;
                    mgr.pendingClick.isPlayer1 = isPlayer1;
                    mgr.pendingClick.maxWaitFrames = cfg.predictionWindow;
                    mgr.pendingClick.elapsedFrames = 0;
                    mgr.pendingClick.needsOrbActivation = false;
                    mgr.pendingClick.needsLanding = true;
                    mgr.pendingClick.targetOrb = nullptr;
                    return; // Buffer click until touchdown!
                }

                // Check 3: Lethal hazard ahead (Wave / Ship / UFO)
                if (!isGroundMode && cfg.predictionMode != "Orb Snapping Only") {
                    bool lethalJump = mgr.willCollideWithHazard(player, playLayer, 2);
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
    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        FramePredictionManager::get().step(this, dt);
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        FramePredictionManager::get().reset();
    }
};
