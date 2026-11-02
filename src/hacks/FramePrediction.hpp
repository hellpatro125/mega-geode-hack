#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

struct PendingClick {
    bool active = false;
    int button = 1;
    bool isPlayer1 = true;
    int targetWaitFrames = 0;
    int maxWaitFrames = 30;
    int elapsedFrames = 0;
    bool needsOrbActivation = false;
    bool needsLanding = false;
    GameObject* targetOrb = nullptr;
};

class FramePredictionManager {
public:
    static FramePredictionManager& get() {
        static FramePredictionManager instance;
        return instance;
    }

    PendingClick pendingClick;
    int totalCorrections = 0;
    int lastDelayFrames = 0;
    float feedbackTimer = 0.0f;

    void reset() {
        pendingClick.active = false;
        pendingClick.targetOrb = nullptr;
    }

    int findSafeJumpDelay(PlayerObject* player, PlayLayer* playLayer, int maxWindowFrames);
    bool willJumpArcHitHazardAtOffset(PlayerObject* player, PlayLayer* playLayer, float offsetX);
    GameObject* findApproachingOrb(PlayerObject* player, PlayLayer* playLayer, int windowFrames);
    void step(PlayLayer* playLayer, float dt);
};
