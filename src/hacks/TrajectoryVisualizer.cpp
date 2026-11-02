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
    float yVel = static_cast<float>(player->m_yVelocity);

    bool isWave = player->m_isDart;
    bool isShip = player->m_isShip;
    bool isBall = player->m_isBall;
    bool isBird = player->m_isBird;
    bool isSwing = player->m_isSwing;

    CCPoint simPos = currentPos;
    float simYVel = yVel;
    bool hitHazard = false;
    CCPoint hitPoint = CCPointZero;

    ccColor4F safeColor = {0.0f, 1.0f, 0.9f, 0.95f};   // Vivid Cyan
    ccColor4F dangerColor = {1.0f, 0.2f, 0.2f, 1.0f}; // Vivid Red

    const int SIM_STEPS = 90;

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
            simPos.y += simYVel * dt;
            simYVel -= 35.0f * dt * gravityDir;
        }

        // Collision check with hazards
        if (!hitHazard && playLayer->m_objects) {
            auto* objects = playLayer->m_objects;
            CCRect simRect(simPos.x - 10.0f, simPos.y - 10.0f, 20.0f, 20.0f);

            for (int i = 0; i < objects->count(); ++i) {
                auto* obj = static_cast<GameObject*>(objects->objectAtIndex(i));
                if (!obj || obj->m_isActivated) continue;

                if (std::abs(obj->getPositionX() - simPos.x) > 60.0f) continue;

                if (obj->m_objectType == GameObjectType::Hazard || obj->m_objectType == GameObjectType::AnimatedHazard) {
                    if (simRect.intersectsRect(obj->getObjectRect())) {
                        hitHazard = true;
                        hitPoint = simPos;
                        break;
                    }
                }
            }
        }

        ccColor4F segColor = hitHazard ? dangerColor : safeColor;
        drawSegmentQuad(m_drawNode, prevPos, simPos, 2.0f, segColor);
        m_drawNode->drawDot(simPos, 2.2f, segColor);
    }

    if (hitHazard) {
        m_drawNode->drawDot(hitPoint, 5.0f, dangerColor);
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
