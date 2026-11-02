#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

class TrajectoryVisualizer {
public:
    static TrajectoryVisualizer& get() {
        static TrajectoryVisualizer instance;
        return instance;
    }

    cocos2d::CCDrawNode* m_drawNode = nullptr;

    void initForPlayLayer(PlayLayer* playLayer);
    void updateTrajectory(PlayLayer* playLayer);
    void clear();
};
