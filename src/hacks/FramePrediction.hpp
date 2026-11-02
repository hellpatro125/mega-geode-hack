#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>

struct PendingClick {
    bool active = false;
    int button = 1;
    bool isPlayer1 = true;
    int maxWaitFrames = 5;
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

    bool isCurrentFrameSafe(PlayerObject* player, PlayLayer* playLayer);
    bool willCollideWithHazard(PlayerObject* player, PlayLayer* playLayer, int framesAhead);
    GameObject* findApproachingOrb(PlayerObject* player, PlayLayer* playLayer, int windowFrames);
    void step(PlayLayer* playLayer, float dt);
};
