#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include "../Config.hpp"
#include "FramePrediction.hpp"
#include <chrono>

using namespace geode::prelude;

static std::vector<std::chrono::steady_clock::time_point> s_clickTimestamps;
static int s_totalJumps = 0;

class $modify(OverlayPlayLayer, PlayLayer) {
    struct Fields {
        cocos2d::CCLabelBMFont* m_overlayLabel = nullptr;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        auto winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();

        auto label = cocos2d::CCLabelBMFont::create("", "bigFont.fnt");
        if (label) {
            label->setScale(0.35f);
            label->setAnchorPoint({0.0f, 1.0f});
            label->setPosition({8.0f, winSize.height - 8.0f});
            label->setZOrder(999);
            this->addChild(label);
            m_fields->m_overlayLabel = label;
        }

        return true;
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);

        auto& cfg = Config::get();
        if (!m_fields->m_overlayLabel) return;

        // Clean up clicks older than 1 second
        auto now = std::chrono::steady_clock::now();
        s_clickTimestamps.erase(
            std::remove_if(s_clickTimestamps.begin(), s_clickTimestamps.end(), [now](auto const& tp) {
                return std::chrono::duration_cast<std::chrono::milliseconds>(now - tp).count() > 1000;
            }),
            s_clickTimestamps.end()
        );

        int currentCPS = static_cast<int>(s_clickTimestamps.size());

        float currentPercent = 0.0f;
        if (m_player1 && !m_isPracticeMode) {
            currentPercent = this->getCurrentPercent();
        }

        std::string hudText = "";

        if (cfg.accuratePercentage) {
            hudText += fmt::format("{:.2f}%\n", currentPercent);
        }

        if (cfg.cpsCounter) {
            hudText += fmt::format("CPS: {} | Jumps: {}\n", currentCPS, s_totalJumps);
        }

        if (cfg.predictionEnabled) {
            auto& mgr = FramePredictionManager::get();
            if (mgr.feedbackTimer > 0.0f) {
                hudText += fmt::format("[Predict: Saved +{}F!]\n", mgr.lastDelayFrames);
            } else {
                hudText += fmt::format("[Predict: Ready (w: {}f)]\n", cfg.predictionWindow);
            }
        }

        if (cfg.safeMode) {
            hudText += "[Safe Mode: ON]";
        }

        m_fields->m_overlayLabel->setString(hudText.c_str());

        if (FramePredictionManager::get().feedbackTimer > 0.0f) {
            m_fields->m_overlayLabel->setColor({50, 255, 120});
        } else {
            m_fields->m_overlayLabel->setColor({255, 255, 255});
        }
    }

    void levelComplete() {
        auto& cfg = Config::get();
        bool cheatActive = cfg.noclip || (cfg.speedhackEnabled && cfg.speedhackValue != 1.0f) || cfg.predictionEnabled;
        if (cfg.safeMode && cheatActive) {
            FLAlertLayer::create("Safe Mode", "Cheat / Frame Prediction used. Score submission blocked.", "OK")->show();
            return;
        }

        PlayLayer::levelComplete();
    }
};

class $modify(CPSBaseGameLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        GJBaseGameLayer::handleButton(down, button, isPlayer1);

        if (down && button == 1 && isPlayer1) {
            s_clickTimestamps.push_back(std::chrono::steady_clock::now());
            s_totalJumps++;
        }
    }
};
