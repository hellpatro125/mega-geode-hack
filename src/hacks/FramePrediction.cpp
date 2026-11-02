#include "FramePrediction.hpp"
#include "../Config.hpp"

using namespace geode::prelude;

static bool s_isDispatchingPredictedClick = false;

static bool isObjectHazard(GameObject* obj, const CCPoint& pos, const CCSize& playerSize) {
    if (!obj) return false;

    CCPoint objPos = obj->getPosition();
    if (std::abs(objPos.x - pos.x) > 45.0f) return false;

    bool isHazard = (obj->m_objectType == GameObjectType::Hazard ||
                     obj->m_objectType == GameObjectType::AnimatedHazard ||
                     obj->m_objectID == 8 || obj->m_objectID == 9 || obj->m_objectID == 39 ||
                     obj->m_objectID == 103 || (obj->m_objectID >= 1733 && obj->m_objectID <= 1736));

    if (isHazard) {
        float dist = ccpDistance(pos, objPos);
        if (dist < 22.0f) return true;

        CCRect playerBox(
            pos.x - playerSize.width * 0.5f,
            pos.y - playerSize.height * 0.5f,
            playerSize.width,
            playerSize.height
        );

        CCRect objRect = obj->getObjectRect();
        if (objRect.size.width < 1.0f) {
            objRect = CCRect(objPos.x - 14.0f, objPos.y - 14.0f, 28.0f, 28.0f);
        }

        if (playerBox.intersectsRect(objRect)) return true;
    }

    return false;
}

bool FramePredictionManager::willJumpArcHitHazardAtOffset(PlayerObject* player, PlayLayer* playLayer, float offsetX) {
    if (!player || !playLayer || !playLayer->m_objects) return false;

    CCPoint startPos = player->getPosition();
    startPos.x += offsetX;

    float gravityDir = player->m_isUpsideDown ? -1.0f : 1.0f;
    float groundY = player->getPositionY();
    CCSize playerSize(24.0f, 24.0f);

    const float jumpLength = 180.0f;
    const float jumpHeight = 66.0f;
    const float stepDist = 5.0f;
    int numSteps = static_cast<int>(jumpLength / stepDist);

    auto* objects = playLayer->m_objects;

    for (int i = 1; i <= numSteps; ++i) {
        float dx = static_cast<float>(i) * stepDist;
        float normX = dx / jumpLength;
        if (normX > 1.0f) normX = 1.0f;

        float arcY = 4.0f * jumpHeight * normX * (1.0f - normX);
        CCPoint simPos(startPos.x + dx, groundY + (arcY * gravityDir));

        for (int j = 0; j < objects->count(); ++j) {
            auto* obj = static_cast<GameObject*>(objects->objectAtIndex(j));
            if (isObjectHazard(obj, simPos, playerSize)) {
                return true;
            }
        }
    }

    return false;
}

int FramePredictionManager::findSafeJumpDelay(PlayerObject* player, PlayLayer* playLayer, int maxWindowFrames) {
    if (!player || !playLayer) return 0;

    // 1. Check if jumping right now is lethal
    bool lethalNow = willJumpArcHitHazardAtOffset(player, playLayer, 0.0f);
    if (!lethalNow) {
        return 0; // Jumping right now is completely safe!
    }

    // 2. Jumping now hits a hazard! Search for earliest safe delay within window:
    float speedMult = player->m_speedMultiplier > 0.0 ? static_cast<float>(player->m_speedMultiplier) : 0.9f;
    float xVel = (speedMult / 0.9f) * 311.58f;
    float dt = 1.0f / 240.0f;

    auto* objects = playLayer->m_objects;
    CCSize playerSize(24.0f, 24.0f);

    for (int k = 1; k <= maxWindowFrames; ++k) {
        float offsetX = xVel * dt * static_cast<float>(k);

        // Check if sliding on ground during this delay runs into a spike:
        CCPoint groundCheckPos = player->getPosition();
        groundCheckPos.x += offsetX;

        bool groundCollision = false;
        if (objects) {
            for (int i = 0; i < objects->count(); ++i) {
                auto* obj = static_cast<GameObject*>(objects->objectAtIndex(i));
                if (isObjectHazard(obj, groundCheckPos, playerSize)) {
                    groundCollision = true;
                    break;
                }
            }
        }
        if (groundCollision) {
            // Cannot delay further, would crash into hazard on ground
            break;
        }

        // Test if jump arc launched at offset clears the hazard:
        if (!willJumpArcHitHazardAtOffset(player, playLayer, offsetX)) {
            return k; // Found safe jump frame!
        }
    }

    return -1; // No safe frame found
}

GameObject* FramePredictionManager::findApproachingOrb(PlayerObject* player, PlayLayer* playLayer, int windowFrames) {
    if (!player || !playLayer || !playLayer->m_objects) return nullptr;

    CCPoint playerPos = player->getPosition();
    float speedMult = player->m_speedMultiplier > 0.0 ? static_cast<float>(player->m_speedMultiplier) : 0.9f;
    float xVel = (speedMult / 0.9f) * 311.58f;
    float dt = 1.0f / 240.0f;
    float maxDistanceAhead = xVel * dt * static_cast<float>(windowFrames) + 12.0f;

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

    bool shouldFire = false;

    if (pendingClick.needsLanding) {
        if (player->m_isOnGround) shouldFire = true;
    } else if (pendingClick.needsOrbActivation && pendingClick.targetOrb) {
        CCRect pRect = player->getObjectRect();
        CCRect oRect = pendingClick.targetOrb->getObjectRect();
        oRect.origin.x -= 16.0f; oRect.origin.y -= 16.0f;
        oRect.size.width += 32.0f; oRect.size.height += 32.0f;
        if (pRect.intersectsRect(oRect)) shouldFire = true;
    } else {
        if (pendingClick.elapsedFrames >= pendingClick.targetWaitFrames) {
            shouldFire = true;
        }
    }

    // Safety timeout: never permanently swallow clicks
    if (pendingClick.elapsedFrames >= pendingClick.maxWaitFrames) {
        shouldFire = true;
    }

    if (shouldFire) {
        lastDelayFrames = pendingClick.elapsedFrames;
        if (pendingClick.elapsedFrames > 0) {
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
        if (s_isDispatchingPredictedClick) {
            GJBaseGameLayer::handleButton(down, button, isPlayer1);
            return;
        }

        auto& cfg = Config::get();
        auto* playLayer = typeinfo_cast<PlayLayer*>(this);

        if (cfg.predictionEnabled && playLayer && button == 1) {
            auto& mgr = FramePredictionManager::get();

            // Release button: flush pending click and send release
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
                // 1. Orb Snapping: delay click until touching approaching orb
                if (cfg.predictionMode != "Hazard Avoidance Only") {
                    GameObject* upcomingOrb = mgr.findApproachingOrb(player, playLayer, cfg.predictionWindow);
                    if (upcomingOrb) {
                        CCRect playerRect = player->getObjectRect();
                        CCRect orbRect = upcomingOrb->getObjectRect();
                        if (!playerRect.intersectsRect(orbRect)) {
                            mgr.pendingClick.active = true;
                            mgr.pendingClick.button = button;
                            mgr.pendingClick.isPlayer1 = isPlayer1;
                            mgr.pendingClick.targetWaitFrames = 0;
                            mgr.pendingClick.maxWaitFrames = cfg.predictionWindow;
                            mgr.pendingClick.elapsedFrames = 0;
                            mgr.pendingClick.needsOrbActivation = true;
                            mgr.pendingClick.needsLanding = false;
                            mgr.pendingClick.targetOrb = upcomingOrb;
                            return;
                        }
                    }
                }

                // 2. Mid-air click before landing (Cube, Ball, Robot, Spider)
                bool isGroundMode = !player->m_isShip && !player->m_isDart && !player->m_isBird && !player->m_isSwing;
                if (isGroundMode && !player->m_isOnGround && player->m_yVelocity <= 0.0) {
                    mgr.pendingClick.active = true;
                    mgr.pendingClick.button = button;
                    mgr.pendingClick.isPlayer1 = isPlayer1;
                    mgr.pendingClick.targetWaitFrames = 0;
                    mgr.pendingClick.maxWaitFrames = cfg.predictionWindow;
                    mgr.pendingClick.elapsedFrames = 0;
                    mgr.pendingClick.needsOrbActivation = false;
                    mgr.pendingClick.needsLanding = true;
                    mgr.pendingClick.targetOrb = nullptr;
                    return;
                }

                // 3. Hazard Avoidance / Click Delay: check if jumping now lands on hazard
                if (cfg.predictionMode != "Orb Snapping Only") {
                    int safeDelay = mgr.findSafeJumpDelay(player, playLayer, cfg.predictionWindow);
                    if (safeDelay > 0) {
                        // Premature click detected! Delaying to safe frame!
                        mgr.pendingClick.active = true;
                        mgr.pendingClick.button = button;
                        mgr.pendingClick.isPlayer1 = isPlayer1;
                        mgr.pendingClick.targetWaitFrames = safeDelay;
                        mgr.pendingClick.maxWaitFrames = cfg.predictionWindow;
                        mgr.pendingClick.elapsedFrames = 0;
                        mgr.pendingClick.needsOrbActivation = false;
                        mgr.pendingClick.needsLanding = false;
                        mgr.pendingClick.targetOrb = nullptr;
                        return; // Delay jump until safe frame!
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
