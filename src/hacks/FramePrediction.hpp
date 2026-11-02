#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>

struct QueuedPredictionClick {
    bool active = false;
    bool isPlayer1 = true;
    int button = 1;
    int maxWaitFrames = 5;
    int elapsedFrames = 0;
    bool needsOrbActivation = false;
    bool needsLanding = false;
    geode::Ref<GameObject> targetOrb = nullptr;
};

class FramePredictionManager {
public:
    static FramePredictionManager& get() {
        static FramePredictionManager instance;
        return instance;
    }

    QueuedPredictionClick pendingClick;
    int totalCorrections = 0;
    int lastDelayFrames = 0;
    float feedbackTimer = 0.0f;

    void reset() {
        pendingClick = QueuedPredictionClick();
        feedbackTimer = 0.0f;
    }

    bool willCollideWithHazard(PlayerObject* player, PlayLayer* playLayer, int framesAhead);
    GameObject* findApproachingOrb(PlayerObject* player, PlayLayer* playLayer, int windowFrames);
    bool isCurrentFrameSafe(PlayerObject* player, PlayLayer* playLayer);
    void step(PlayLayer* playLayer, float dt);
};
