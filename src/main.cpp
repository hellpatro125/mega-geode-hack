#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include "Config.hpp"
#include "ui/HackMenuLayer.hpp"

using namespace geode::prelude;

$on_mod(Loaded) {
    Config::get().loadFromSettings();
    log::info("MegaHack Geode & Frame Prediction loaded successfully!");
}

class $modify(MegaMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        // Add a MegaHack button to the bottom menu
        if (auto bottomMenu = this->getChildByID("bottom-menu")) {
            auto spr = ButtonSprite::create("MH", "goldFont.fnt", "GJ_button_01.png", 0.7f);
            auto btn = CCMenuItemSpriteExtra::create(
                spr, this, menu_selector(MegaMenuLayer::onOpenMegaHack)
            );
            btn->setID("megahack-button"_spr);
            bottomMenu->addChild(btn);
            bottomMenu->updateLayout();
        }

        return true;
    }

    void onOpenMegaHack(CCObject*) {
        if (auto popup = HackMenuLayer::create()) {
            popup->show();
        }
    }
};

class $modify(MegaPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        // Add MegaHack button to pause screen
        if (auto rightMenu = this->getChildByID("right-button-menu")) {
            auto spr = ButtonSprite::create("MH", "goldFont.fnt", "GJ_button_02.png", 0.7f);
            auto btn = CCMenuItemSpriteExtra::create(
                spr, this, menu_selector(MegaPauseLayer::onOpenMegaHackPause)
            );
            btn->setID("megahack-pause-button"_spr);
            rightMenu->addChild(btn);
            rightMenu->updateLayout();
        }
    }

    void onOpenMegaHackPause(CCObject*) {
        if (auto popup = HackMenuLayer::create()) {
            popup->show();
        }
    }
};

#if defined(GEODE_IS_WINDOWS) || defined(GEODE_IS_MACOS)
class $modify(MegaKeyboardDispatcher, CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(enumKeyCodes key, bool isKeyDown, bool isKeyRepeat) {
        // Shift key (Shift, LeftShift, RightShift) or Tab toggles the Hack Menu
        if ((key == KEY_Shift || key == KEY_LeftShift || key == KEY_RightShift || key == KEY_Tab) && isKeyDown && !isKeyRepeat) {
            auto scene = CCDirector::sharedDirector()->getRunningScene();
            if (scene) {
                if (auto existing = scene->getChildByID("HackMenuLayer")) {
                    // Close menu if already open
                    existing->removeFromParentAndCleanup(true);
                    return true;
                }
                if (auto popup = HackMenuLayer::create()) {
                    popup->setID("HackMenuLayer");
                    popup->show();
                    return true;
                }
            }
        }
        return CCKeyboardDispatcher::dispatchKeyboardMSG(key, isKeyDown, isKeyRepeat);
    }
};
#endif
