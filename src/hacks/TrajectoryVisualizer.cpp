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

    CCPoint verts[4] = {
        ccpAdd(from, perp),
        ccpSub(from, perp),
        ccpSub(to, perp),
        ccpAdd(to, perp)
    };

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
        if (std::abs(objPos.x - pos.x) > 40.0f) continue;

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
    float gravityDir = player->m_isUpsideDown ? -1.0f : 1.0f;
    float groundY = currentPos.y;
    CCSize playerSize(26.0f, 26.0f);

    ccColor4F redColor = {1.0f, 0.15f, 0.15f, 1.0f};          // Red Danger Line
    ccColor4F greenColor = {0.0f, 1.0f, 0.15f, 1.0f};        // Neon Green Jump Arc
    ccColor4F pinkFill = {1.0f, 0.15f, 0.65f, 0.55f};        // Pink Death Box Fill
    ccColor4F pinkBorder = {1.0f, 0.1f, 0.3f, 1.0f};         // Red Death Box Border

    bool isCube = player->isInNormalMode();
    bool isWave = player->m_isDart;
    bool isShip = player->isFlying();

    // =========================================================================
    // 1. DANGER CHECK: Line straight ahead on floor to spike
    // =========================================================================
    {
        CCPoint simPos = currentPos;
        bool hitHazard = false;
        CCPoint hitPos = CCPointZero;

        // Check up to 220 pixels ahead along the ground
        const float MAX_LOOKAHEAD = 220.0f;
        const float STEP_X = 4.0f;
        int maxSteps = static_cast<int>(MAX_LOOKAHEAD / STEP_X);

        for (int step = 1; step <= maxSteps; ++step) {
            CCPoint prevPos = simPos;
            simPos.x += STEP_X;

            if (checkHazardAt(simPos, playerSize, playLayer)) {
                hitHazard = true;
                hitPos = simPos;
                drawSegmentQuad(m_drawNode, prevPos, simPos, 2.0f, redColor);
                break;
            }

            drawSegmentQuad(m_drawNode, prevPos, simPos, 1.6f, redColor);
        }

        // Draw Player Death Hitbox (Pink square with red border and center dot) on the hazard
        if (hitHazard) {
            drawHitboxBox(m_drawNode, hitPos, playerSize, pinkFill, pinkBorder);
        }
    }

    // =========================================================================
    // 2. JUMP PATH: Exact Mathematical Parabola (Jump Arc over obstacles)
    // =========================================================================
    if (isCube) {
        // Standard GD Cube jump: Length = 180px, Height = 66px
        const float jumpLength = 180.0f;
        const float jumpHeight = 66.0f;
        const float stepDist = 3.5f;
        int numSteps = static_cast<int>(jumpLength / stepDist);

        CCPoint prevPos = currentPos;
        bool hitHazard = false;
        CCPoint hitPos = CCPointZero;

        for (int i = 1; i <= numSteps; ++i) {
            float dx = static_cast<float>(i) * stepDist;
            float normX = dx / jumpLength;
            if (normX > 1.0f) normX = 1.0f;

            // Parabolic arc: y = groundY + 4 * H * normX * (1 - normX)
            float arcY = 4.0f * jumpHeight * normX * (1.0f - normX);
            CCPoint simPos(currentPos.x + dx, groundY + (arcY * gravityDir));

            if (checkHazardAt(simPos, playerSize, playLayer)) {
                hitHazard = true;
                hitPos = simPos;
                drawSegmentQuad(m_drawNode, prevPos, simPos, 1.8f, redColor);
                break;
            }

            drawSegmentQuad(m_drawNode, prevPos, simPos, 1.8f, greenColor);
            prevPos = simPos;
        }

        if (hitHazard) {
            drawHitboxBox(m_drawNode, hitPos, playerSize, pinkFill, pinkBorder);
        }
    } else if (isWave) {
        // Wave 45-degree angle path
        float length = 180.0f;
        CCPoint upEnd(currentPos.x + length, currentPos.y + (length * gravityDir));
        drawSegmentQuad(m_drawNode, currentPos, upEnd, 1.8f, greenColor);
    } else if (isShip) {
        // Ship upward curved thrust path
        CCPoint prevPos = currentPos;
        for (int i = 1; i <= 30; ++i) {
            float dx = i * 6.0f;
            float dy = (i * 3.5f) * gravityDir;
            CCPoint simPos(currentPos.x + dx, currentPos.y + dy);
            drawSegmentQuad(m_drawNode, prevPos, simPos, 1.8f, greenColor);
            prevPos = simPos;
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
