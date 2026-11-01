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

    // Checks whether an immediate jump at current position causes hazard collision
    bool willCollideWithHazard(PlayerObject* player, PlayLayer* playLayer, int framesAhead);

    // Checks if the player is approaching an interactive ring/orb within the frame prediction window
    GameObject* findApproachingOrb(PlayerObject* player, PlayLayer* playLayer, int windowFrames);

    // Tests if current frame is safe to release the jump
    bool isCurrentFrameSafe(PlayerObject* player, PlayLayer* playLayer);

    // Handles the queued click step each game tick
    void step(PlayLayer* playLayer, float dt);
};
