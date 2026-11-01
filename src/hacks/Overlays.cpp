#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include "../Config.hpp"
#include "FramePrediction.hpp"

using namespace geode::prelude;

class $modify(OverlayPlayLayer, PlayLayer) {
    struct Fields {
        cocos2d::CCLabelBMFont* m_overlayLabel = nullptr;
        std::vector<float> m_clickTimestamps;
        int m_totalJumps = 0;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        auto winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();

        // Create status HUD label in top-left or bottom-left corner
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

    void update(float dt) {
        PlayLayer::update(dt);

        auto& cfg = Config::get();
        if (!m_fields->m_overlayLabel) return;

        // Clean up old click timestamps (older than 1 second)
        float now = this->m_time;
        auto& clicks = m_fields->m_clickTimestamps;
        clicks.erase(
            std::remove_if(clicks.begin(), clicks.end(), [now](float t) {
                return (now - t) > 1.0f;
            }),
            clicks.end()
        );

        int currentCPS = static_cast<int>(clicks.size());

        // Accurate percentage calculation
        float currentPercent = 0.0f;
        if (m_player1 && !m_isPracticeMode) {
            currentPercent = this->getCurrentPercent();
        }

        std::string hudText = "";

        if (cfg.accuratePercentage) {
            hudText += fmt::format("{:.2f}%\n", currentPercent);
        }

        if (cfg.cpsCounter) {
            hudText += fmt::format("CPS: {} | Jumps: {}\n", currentCPS, m_fields->m_totalJumps);
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

        // Color coding: green when frame prediction triggered, white normally
        if (FramePredictionManager::get().feedbackTimer > 0.0f) {
            m_fields->m_overlayLabel->setColor({50, 255, 120});
        } else {
            m_fields->m_overlayLabel->setColor({255, 255, 255});
        }
    }

    void levelComplete() {
        auto& cfg = Config::get();
        // Safe Mode: prevent submitting leaderboard/saving progress if hacks were active
        bool cheatActive = cfg.noclip || (cfg.speedhackEnabled && cfg.speedhackValue != 1.0f) || cfg.predictionEnabled;
        if (cfg.safeMode && cheatActive) {
            FLAlertLayer::create("Safe Mode", "Уровень пройден с активными читами / Frame Prediction. Прогресс не отправлен на сервер.", "OK")->show();
            return;
        }

        PlayLayer::levelComplete();
    }
};

class $modify(CPSBaseGameLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        GJBaseGameLayer::handleButton(down, button, isPlayer1);

        if (down && button == 1 && isPlayer1) {
            if (auto* playLayer = typeinfo_cast<PlayLayer*>(this)) {
                auto* fields = static_cast<OverlayPlayLayer::Fields*>(playLayer->m_fields.self());
                if (fields) {
                    fields->m_clickTimestamps.push_back(playLayer->m_time);
                    fields->m_totalJumps++;
                }
            }
        }
    }
};
