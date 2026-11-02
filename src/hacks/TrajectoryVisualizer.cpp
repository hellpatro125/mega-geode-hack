#include "TrajectoryVisualizer.hpp"
#include "../Config.hpp"

using namespace geode::prelude;

void TrajectoryVisualizer::initForPlayLayer(PlayLayer* playLayer) {
    if (!playLayer || !playLayer->m_objectLayer) return;

    m_drawNode = cocos2d::CCDrawNode::create();
    if (m_drawNode) {
        m_drawNode->setZOrder(60);
        playLayer->m_objectLayer->addChild(m_drawNode);
    }
}

void TrajectoryVisualizer::clear() {
    if (m_drawNode) {
        m_drawNode->clear();
    }
}

void TrajectoryVisualizer::updateTrajectory(PlayLayer* playLayer) {
    if (!Config::get().showTrajectory || !playLayer || !m_drawNode) {
        clear();
        return;
    }

    auto* player = playLayer->m_player1;
    if (!player || player->m_isDead) {
        clear();
        return;
    }

    m_drawNode->clear();

    cocos2d::CCPoint currentPos = player->getPosition();
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

    cocos2d::CCPoint simPos = currentPos;
    float simYVel = yVel;
    bool hitHazard = false;
    cocos2d::CCPoint hitPoint;

    cocos2d::ccColor4F safeColor = {0.0f, 0.9f, 1.0f, 0.85f};   // Glowing Cyan
    cocos2d::ccColor4F dangerColor = {1.0f, 0.2f, 0.2f, 0.95f}; // Bright Red warning

    const int SIM_STEPS = 90; // Forward trajectory lookahead

    for (int step = 1; step <= SIM_STEPS; ++step) {
        cocos2d::CCPoint prevPos = simPos;

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
            // Cube / Robot
            simPos.x += xVel * dt;
            simPos.y += simYVel * dt;
            simYVel -= 35.0f * dt * gravityDir;
        }

        // Detect hazard collision along predicted path
        if (!hitHazard && playLayer->m_objects) {
            auto* objects = playLayer->m_objects;
            cocos2d::CCRect simRect(simPos.x - 12.0f, simPos.y - 12.0f, 24.0f, 24.0f);

            for (int i = 0; i < objects->count(); ++i) {
                auto* obj = static_cast<GameObject*>(objects->objectAtIndex(i));
                if (!obj || obj->m_isActivated) continue;

                if (std::abs(obj->getPositionX() - simPos.x) > 80.0f) continue;

                if (obj->m_objectType == GameObjectType::Hazard) {
                    if (simRect.intersectsRect(obj->getObjectRect())) {
                        hitHazard = true;
                        hitPoint = simPos;
                        break;
                    }
                }
            }
        }

        cocos2d::ccColor4F segColor = hitHazard ? dangerColor : safeColor;
        m_drawNode->drawSegment(prevPos, simPos, 1.6f, segColor);
    }

    if (hitHazard) {
        m_drawNode->drawDot(hitPoint, 4.0f, dangerColor);
    }
}

class $modify(TrajectoryPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        TrajectoryVisualizer::get().initForPlayLayer(this);
        return true;
    }

    void update(float dt) {
        PlayLayer::update(dt);
        TrajectoryVisualizer::get().updateTrajectory(this);
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        TrajectoryVisualizer::get().clear();
    }
};
