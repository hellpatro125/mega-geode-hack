#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "../Config.hpp"

using namespace geode::prelude;

class $modify(NoclipPlayLayer, PlayLayer) {
    struct Fields {
        int m_noclipDeaths = 0;
    };

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (Config::get().noclip) {
            m_fields->m_noclipDeaths++;
            // Do not call original destroyPlayer = player survives!
            return;
        }

        PlayLayer::destroyPlayer(player, object);
    }
};
