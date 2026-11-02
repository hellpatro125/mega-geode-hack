#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include "../Config.hpp"

class HackMenuLayer : public geode::Popup {
protected:
    bool init(float width, float height);

    cocos2d::CCLabelBMFont* m_windowLabel = nullptr;
    cocos2d::CCLabelBMFont* m_speedLabel = nullptr;

    void onTogglePrediction(cocos2d::CCObject* sender);
    void onWindowMinus(cocos2d::CCObject* sender);
    void onWindowPlus(cocos2d::CCObject* sender);

    void onToggleNoclip(cocos2d::CCObject* sender);
    void onToggleTrajectory(cocos2d::CCObject* sender);
    void onToggleSpeedhack(cocos2d::CCObject* sender);
    void onSpeedMinus(cocos2d::CCObject* sender);
    void onSpeedPlus(cocos2d::CCObject* sender);

    void onTogglePracticeMusic(cocos2d::CCObject* sender);
    void onToggleAccuratePercent(cocos2d::CCObject* sender);
    void onToggleCPS(cocos2d::CCObject* sender);
    void onToggleSafeMode(cocos2d::CCObject* sender);

public:
    static HackMenuLayer* create();
};
