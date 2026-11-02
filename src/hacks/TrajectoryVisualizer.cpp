#include "TrajectoryVisualizer.hpp"
#include "../Config.hpp"

using namespace geode::prelude;

static void drawSegmentQuad(CCDrawNode* node, const CCPoint& from, const CCPoint& to, float radius, const ccColor4F& color) {
    if (!node) return;
    CCPoint diff = ccpSub(to, from);
    float len = ccpLength(diff);
    if (len < 0.001f) return;

    CCPoint dir = ccpMult(diff, 1.0f / len);
    CCPoint perp = ccp(-dir.y * radius, dir.x * radius);

    CCPoint verts[4];
    verts[0] = ccpAdd(from, perp);
    verts[1] = ccpSub(from, perp);
    verts[2] = ccpSub(to, perp);
    verts[3] = ccpAdd(to, perp);

    node->drawPolygon(verts, 4, color, 0.0f, color);
}

static void drawHitboxBox(CCDrawNode* node, const CCPoint& center, const CCSize& size, const ccColor4F& fillColor, const ccColor4F& borderColor) {
    if (!node) return;

    float halfW = size.width * 0.5f;
    float halfH = size.height * 0.5f;

    CCPoint verts[4] = {
        CCPoint(center.x - halfW, center.y - halfH),
        CCPoint(center.x + halfW, center.y - halfH),
        CCPoint(center.x + halfW, center.y + halfH),
        CCPoint(center.x - halfW, center.y + halfH)
    };

    node->drawPolygon(verts, 4, fillColor, 1.8f, borderColor);

    // Inner green/white center marker
    float dotR = 2.5f;
    CCPoint dotVerts[4] = {
        CCPoint(center.x - dotR, center.y - dotR),
        CCPoint(center.x + dotR, center.y - dotR),
        CCPoint(center.x + dotR, center.y + dotR),
        CCPoint(center.x - dotR, center.y + dotR)
    };

    ccColor4F dotColor = {0.2f, 1.0f, 0.4f, 1.0f};
    node->drawPolygon(dotVerts, 4, dotColor, 0.5f, {1.0f, 1.0f, 1.0f, 1.0f});
}

static bool checkHazardAt(const CCPoint& pos, const CCSize& playerSize, PlayLayer* playLayer) {
    if (!playLayer || !playLayer->m_objects) return false;

    CCRect playerBox(
        pos.x - playerSize.width * 0.5f,
        pos.y - playerSize.height * 0.5f,
        playerSize.width,
        playerSize.height
    );

    auto* objects = playLayer->m_objects;
    for (int i = 0; i < objects->count(); ++i) {
        auto* obj = static_cast<GameObject*>(objects->objectAtIndex(i));
        if (!obj) continue;

        CCPoint objPos = obj->getPosition();
        if (std::abs(objPos.x - pos.x) > 45.0f) continue;

        bool isHazard = (obj->m_objectType == GameObjectType::Hazard ||
                         obj->m_objectType == GameObjectType::AnimatedHazard ||
                         obj->m_objectID == 8 || obj->m_objectID == 9 || obj->m_objectID == 39 ||
                         obj->m_objectID == 103 || (obj->m_objectID >= 1733 && obj->m_objectID <= 1736));

        if (isHazard) {
            float dist = ccpDistance(pos, objPos);
            if (dist < 22.0f) {
                return true;
            }

            CCRect objRect = obj->getObjectRect();
            if (objRect.size.width < 1.0f) {
                objRect = CCRect(objPos.x - 14.0f, objPos.y - 14.0f, 28.0f, 28.0f);
            }

            if (playerBox.intersectsRect(objRect)) {
                return true;
            }
        }
    }
    return false;
}

void TrajectoryVisualizer::setupForPlayLayer(PlayLayer* playLayer) {
    if (!playLayer) return;

    if (m_drawNode) {
        m_drawNode->removeFromParentAndCleanup(true);
        m_drawNode = nullptr;
    }

    m_drawNode = CCDrawNode::create();
    if (!m_drawNode) return;

    m_drawNode->setBlendFunc(ccBlendFunc({ GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA }));
    m_drawNode->setZOrder(9999);

    if (playLayer->m_debugDrawNode && playLayer->m_debugDrawNode->getParent()) {
        playLayer->m_debugDrawNode->getParent()->addChild(m_drawNode, 9999);
    } else if (playLayer->m_objectLayer && playLayer->m_objectLayer->getParent()) {
        playLayer->m_objectLayer->getParent()->addChild(m_drawNode, 9999);
    } else {
        playLayer->addChild(m_drawNode, 9999);
    }
}

void TrajectoryVisualizer::clear() {
    if (m_drawNode) {
        m_drawNode->clear();
    }
}

void TrajectoryVisualizer::updateTrajectory(PlayLayer* playLayer) {
    if (!Config::get().showTrajectory || !playLayer) {
        clear();
        return;
    }

    auto* player = playLayer->m_player1;
    if (!player || player->m_isDead) {
        clear();
        return;
    }

    if (!m_drawNode || !m_drawNode->getParent()) {
        setupForPlayLayer(playLayer);
        if (!m_drawNode) return;
    }

    m_drawNode->clear();

    CCPoint currentPos = player->getPosition();
    float speedMult = player->m_speedMultiplier > 0.0 ? static_cast<float>(player->m_speedMultiplier) : 0.9f;
    float xVel = (speedMult / 0.9f) * 311.58f;
    float dt = 1.0f / 240.0f;
    float gravityDir = player->m_isUpsideDown ? -1.0f : 1.0f;

    bool isWave = player->m_isDart;
    bool isShip = player->m_isShip;
    bool isBall = player->m_isBall;
    bool isBird = player->m_isBird;
    bool isSwing = player->m_isSwing;
    bool isGroundMode = (!isWave && !isShip && !isBird && !isSwing);

    float groundY = currentPos.y;
    CCSize playerSize(26.0f, 26.0f);

    ccColor4F redColor = {1.0f, 0.15f, 0.15f, 1.0f};          // Bright Red Line
    ccColor4F greenColor = {0.0f, 1.0f, 0.15f, 1.0f};        // Pure Neon Green Arc
    ccColor4F pinkFill = {1.0f, 0.15f, 0.65f, 0.55f};        // Pink Translucent Hitbox Fill
    ccColor4F pinkBorder = {1.0f, 0.1f, 0.3f, 1.0f};         // Red/Pink Hitbox Border

    // =========================================================================
    // 1. NO-CLICK / RELEASE PATH (Straight Red Line to Hazard + Death Hitbox)
    // =========================================================================
    {
        CCPoint simPos = currentPos;
        float simYVel = static_cast<float>(player->m_yVelocity);
        bool hitHazard = false;
        CCPoint hitPos = CCPointZero;

        const int RELEASE_STEPS = 200;

        for (int step = 1; step <= RELEASE_STEPS; ++step) {
            CCPoint prevPos = simPos;

            if (isWave) {
                float dirY = -1.0f; // falling diagonally
                simPos.x += xVel * dt;
                simPos.y += dirY * xVel * dt * gravityDir;
            } else if (isShip) {
                simPos.x += xVel * dt;
                simPos.y += simYVel * dt;
                simYVel -= 18.0f * 60.0f * dt * gravityDir;
            } else if (isBall) {
                simPos.x += xVel * dt;
                simPos.y += simYVel * dt;
                simYVel -= 35.0f * 60.0f * dt * gravityDir;
            } else if (isBird) {
                simPos.x += xVel * dt;
                simPos.y += simYVel * dt;
                simYVel -= 25.0f * 60.0f * dt * gravityDir;
            } else if (isSwing) {
                simPos.x += xVel * dt;
                simPos.y += simYVel * dt;
                simYVel -= 15.0f * 60.0f * dt * gravityDir;
            } else {
                // Cube / Robot / Spider
                simPos.x += xVel * dt;
                if (player->m_isOnGround) {
                    simPos.y = groundY;
                    simYVel = 0.0f;
                } else {
                    simPos.y += simYVel * dt;
                    simYVel -= 35.0f * 60.0f * dt * gravityDir;
                    if (simPos.y < groundY) {
                        simPos.y = groundY;
                        simYVel = 0.0f;
                    }
                }
            }

            if (checkHazardAt(simPos, playerSize, playLayer)) {
                hitHazard = true;
                hitPos = simPos;
                drawSegmentQuad(m_drawNode, prevPos, simPos, 2.0f, redColor);
                break; // Stop right at the hazard!
            }

            drawSegmentQuad(m_drawNode, prevPos, simPos, 2.0f, redColor);
        }

        // Draw Player Death Hitbox (Pink square with red border and center dot)
        if (hitHazard) {
            drawHitboxBox(m_drawNode, hitPos, playerSize, pinkFill, pinkBorder);
        }
    }

    // =========================================================================
    // 2. JUMP / HOLD PATH (Smooth Green Parabolas across multiple bounces)
    // =========================================================================
    if (isGroundMode) {
        CCPoint simPos = currentPos;
        // RobTop cube jump physics: impulse 455 px/s, gravity -1570 px/s^2 (height ~66px, length ~180px)
        float jumpImpulse = 455.0f * gravityDir;
        float gravityAcc = -1570.0f * gravityDir;
        float simYVel = jumpImpulse;

        int bounces = 0;
        const int MAX_BOUNCES = 3; // 3 continuous jump parabolas
        const int MAX_TOTAL_STEPS = 550;
        bool hitHazard = false;
        CCPoint hitPos = CCPointZero;

        for (int step = 1; step <= MAX_TOTAL_STEPS; ++step) {
            CCPoint prevPos = simPos;

            simPos.x += xVel * dt;
            simPos.y += simYVel * dt;
            simYVel += gravityAcc * dt;

            // Landing on the floor
            if ((gravityDir > 0.0f && simPos.y <= groundY && step > 6) ||
                (gravityDir < 0.0f && simPos.y >= groundY && step > 6)) {
                simPos.y = groundY;
                bounces++;
                if (bounces >= MAX_BOUNCES) {
                    drawSegmentQuad(m_drawNode, prevPos, simPos, 1.8f, greenColor);
                    break;
                }
                // Bounce into next jump arc!
                simYVel = jumpImpulse;
            }

            if (checkHazardAt(simPos, playerSize, playLayer)) {
                hitHazard = true;
                hitPos = simPos;
                drawSegmentQuad(m_drawNode, prevPos, simPos, 1.8f, redColor);
                break;
            }

            drawSegmentQuad(m_drawNode, prevPos, simPos, 1.8f, greenColor);
        }

        if (hitHazard) {
            drawHitboxBox(m_drawNode, hitPos, playerSize, pinkFill, pinkBorder);
        }
    } else if (isBird || isShip || isWave || isSwing || isBall) {
        // Continuous green arc / wave path for other gamemodes
        CCPoint simPos = currentPos;
        float simYVel = 0.0f;
        if (isWave) {
            simYVel = xVel * gravityDir;
        } else if (isShip) {
            simYVel = static_cast<float>(player->m_yVelocity) + (400.0f * gravityDir);
        } else if (isBird) {
            simYVel = 600.0f * gravityDir;
        }

        bool hitHazard = false;
        CCPoint hitPos = CCPointZero;

        for (int step = 1; step <= 250; ++step) {
            CCPoint prevPos = simPos;

            if (isWave) {
                simPos.x += xVel * dt;
                simPos.y += simYVel * dt;
            } else if (isShip) {
                simPos.x += xVel * dt;
                simPos.y += simYVel * dt;
                simYVel -= 1080.0f * dt * gravityDir;
            } else if (isBird) {
                simPos.x += xVel * dt;
                simPos.y += simYVel * dt;
                simYVel -= 1500.0f * dt * gravityDir;
            } else {
                simPos.x += xVel * dt;
                simPos.y += simYVel * dt;
            }

            if (checkHazardAt(simPos, playerSize, playLayer)) {
                hitHazard = true;
                hitPos = simPos;
                drawSegmentQuad(m_drawNode, prevPos, simPos, 1.8f, redColor);
                break;
            }

            drawSegmentQuad(m_drawNode, prevPos, simPos, 1.8f, greenColor);
        }

        if (hitHazard) {
            drawHitboxBox(m_drawNode, hitPos, playerSize, pinkFill, pinkBorder);
        }
    }
}

class $modify(TrajectoryPlayLayer, PlayLayer) {
    void createObjectsFromSetupFinished() {
        PlayLayer::createObjectsFromSetupFinished();
        TrajectoryVisualizer::get().setupForPlayLayer(this);
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        TrajectoryVisualizer::get().updateTrajectory(this);
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        TrajectoryVisualizer::get().clear();
    }
};
