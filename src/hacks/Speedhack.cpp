#include <Geode/Geode.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(SpeedhackScheduler, cocos2d::CCScheduler) {
    void update(float dt) {
        auto& cfg = Config::get();
        if (cfg.speedhackEnabled && cfg.speedhackValue > 0.0f) {
            CCScheduler::update(dt * cfg.speedhackValue);
        } else {
            CCScheduler::update(dt);
        }
    }
};
