#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(PracticeMusicPlayLayer, PlayLayer) {
    void togglePracticeMode(bool practice) {
        if (!Config::get().practiceMusic) {
            PlayLayer::togglePracticeMode(practice);
            return;
        }

        bool wasPractice = m_isPracticeMode;
        PlayLayer::togglePracticeMode(practice);

        if (practice && !wasPractice) {
            // Restart / resume level music in practice mode
            this->startMusic();
        }
    }
};
