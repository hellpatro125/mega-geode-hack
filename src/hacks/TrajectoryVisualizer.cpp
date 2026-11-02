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

static bool checkHazardCollision(const CCPoint& pos, PlayLayer* playLayer) {
    if (!playLayer || !playLayer->m_objects) return false;

    auto* objects = playLayer->m_objects;
    for (int i = 0; i < objects->count(); ++i) {
        auto* obj = static_cast<GameObject*>(objects->objectAtIndex(i));
        if (!obj) continue;

        // Skip distant objects
        CCPoint objPos = obj->getPosition();
        if (std::abs(objPos.x - pos.x) > 40.0f) continue;

        bool isHazard = (obj->m_objectType == GameObjectType::Hazard ||
                         obj->m_objectType == GameObjectType::AnimatedHazard ||
                         obj->m_objectID == 8 || obj->m_objectID == 9 || obj->m_objectID == 39 ||
                         obj->m_objectID == 103 || (obj->m_objectID >= 1733 && obj->m_objectID <= 1736));

        if (isHazard) {
            float dist = ccpDistance(pos, objPos);
            if (dist < 24.0f) {
                return true;
            }

            CCRect objRect = obj->getObjectRect();
            if (objRect.size.width < 1.0f) {
                objRect = CCRect(objPos.x - 14.0f, objPos.y - 14.0f, 28.0f, 28.0f);
            }
            CCRect testRect(pos.x - 12.0f, pos.y - 12.0f, 24.0f, 24.0f);
            if (testRect.intersectsRect(objRect)) {
                return true;
            }
        }
    }
    return false;
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
    const int SIM_STEPS = 80;

    ccColor4F dangerColor = {1.0f, 0.15f, 0.15f, 1.0f}; // Bright Red warning

    // ==========================================
    // 1. Current / Default Path (Cyan)
    // ==========================================
    {
        CCPoint simPos = currentPos;
        float simYVel = static_cast<float>(player->m_yVelocity);
        bool hitHazard = false;
        CCPoint hitPoint = CCPointZero;
        ccColor4F pathColor = {0.0f, 0.95f, 1.0f, 0.95f}; // Vivid Cyan

        for (int step = 1; step <= SIM_STEPS; ++step) {
            CCPoint prevPos = simPos;

            if (isWave) {
                float dirY = (player->m_yVelocity >= 0.0) ? 1.0f : -1.0f;
                simPos.x += xVel * dt;
                simPos.y += dirY * xVel * dt * gravityDir;
            } else if (isShip) {
                simPos.x += xVel * dt;
                simPos.y += simYVel * dt;
                simYVel -= 18.0f * dt * gravityDir;
            } else if (isBall) {
                simPos.x += xVel * dt;
                simPos.y += simYVel * dt;
                simYVel -= 35.0f * dt * gravityDir;
            } else if (isBird) {
                simPos.x += xVel * dt;
                simPos.y += simYVel * dt;
                simYVel -= 25.0f * dt * gravityDir;
            } else if (isSwing) {
                simPos.x += xVel * dt;
                simPos.y += simYVel * dt;
                simYVel -= 15.0f * dt * gravityDir;
            } else {
                // Cube / Robot / Spider
                simPos.x += xVel * dt;
                if (player->m_isOnGround) {
                    simPos.y = groundY;
                    simYVel = 0.0f;
                } else {
                    simPos.y += simYVel * dt;
                    simYVel -= 35.0f * dt * gravityDir;
                }
            }

            if (!hitHazard && checkHazardCollision(simPos, playLayer)) {
                hitHazard = true;
                hitPoint = simPos;
            }

            ccColor4F segColor = hitHazard ? dangerColor : pathColor;
            drawSegmentQuad(m_drawNode, prevPos, simPos, 2.0f, segColor);
            m_drawNode->drawDot(simPos, 2.2f, segColor);
        }

        if (hitHazard) {
            m_drawNode->drawDot(hitPoint, 5.5f, dangerColor);
        }
    }

    // ==========================================
    // 2. Jump / Click Path (Green / Yellow Arc)
    // ==========================================
    if (isGroundMode || isBird || isShip) {
        CCPoint simPos = currentPos;
        float simYVel = 0.0f;
        if (isGroundMode) {
            simYVel = 12.0f * gravityDir; // Jump impulse
        } else if (isBird) {
            simYVel = 10.5f * gravityDir;
        } else if (isShip) {
            simYVel = static_cast<float>(player->m_yVelocity) + (18.0f * gravityDir);
        }

        bool hitHazard = false;
        CCPoint hitPoint = CCPointZero;
        ccColor4F jumpColor = {0.2f, 1.0f, 0.35f, 0.9f}; // Bright Green Arc

        for (int step = 1; step <= SIM_STEPS; ++step) {
            CCPoint prevPos = simPos;

            simPos.x += xVel * dt;
            simPos.y += simYVel * dt;

            if (isShip) {
                simYVel -= 18.0f * dt * gravityDir;
            } else if (isBird) {
                simYVel -= 25.0f * dt * gravityDir;
            } else {
                simYVel -= 35.0f * dt * gravityDir;
                // Land back on ground level
                if (player->m_isOnGround && simPos.y < groundY && step > 5) {
                    simPos.y = groundY;
                    simYVel = 0.0f;
                }
            }

            if (!hitHazard && checkHazardCollision(simPos, playLayer)) {
                hitHazard = true;
                hitPoint = simPos;
            }

            ccColor4F segColor = hitHazard ? dangerColor : jumpColor;
            drawSegmentQuad(m_drawNode, prevPos, simPos, 1.6f, segColor);
            m_drawNode->drawDot(simPos, 1.8f, segColor);
        }

        if (hitHazard) {
            m_drawNode->drawDot(hitPoint, 4.5f, dangerColor);
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
