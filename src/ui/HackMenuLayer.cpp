#include "HackMenuLayer.hpp"

using namespace geode::prelude;

HackMenuLayer* HackMenuLayer::create() {
    auto ret = new HackMenuLayer();
    if (ret && ret->init(380.0f, 290.0f)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool HackMenuLayer::init(float width, float height) {
    if (!geode::Popup::init(width, height, "GJ_square01.png")) return false;

    this->setTitle("MegaHack & Frame Prediction");

    auto& cfg = Config::get();
    auto menu = this->m_mainLayer;

    // 1. Frame Prediction Toggle & Window
    auto predToggle = CCMenuItemToggler::createWithStandardSprites(
        this, menu_selector(HackMenuLayer::onTogglePrediction), 0.7f
    );
    predToggle->toggle(cfg.predictionEnabled);
    predToggle->setPosition({45.0f, 220.0f});
    this->m_buttonMenu->addChild(predToggle);

    auto predTitle = CCLabelBMFont::create("Frame Prediction", "bigFont.fnt");
    predTitle->setScale(0.4f);
    predTitle->setAnchorPoint({0.0f, 0.5f});
    predTitle->setPosition({70.0f, 220.0f});
    menu->addChild(predTitle);

    // Frame window buttons [-] [ 5 frames ] [+]
    auto minusBtnSpr = ButtonSprite::create("-", "goldFont.fnt", "GJ_button_01.png", 0.6f);
    minusBtnSpr->setScale(0.7f);
    auto minusBtn = CCMenuItemSpriteExtra::create(
        minusBtnSpr, this, menu_selector(HackMenuLayer::onWindowMinus)
    );
    minusBtn->setPosition({230.0f, 220.0f});
    this->m_buttonMenu->addChild(minusBtn);

    m_windowLabel = CCLabelBMFont::create(
        fmt::format("{} frames", cfg.predictionWindow).c_str(), "bigFont.fnt"
    );
    m_windowLabel->setScale(0.35f);
    m_windowLabel->setPosition({285.0f, 220.0f});
    menu->addChild(m_windowLabel);

    auto plusBtnSpr = ButtonSprite::create("+", "goldFont.fnt", "GJ_button_01.png", 0.6f);
    plusBtnSpr->setScale(0.7f);
    auto plusBtn = CCMenuItemSpriteExtra::create(
        plusBtnSpr, this, menu_selector(HackMenuLayer::onWindowPlus)
    );
    plusBtn->setPosition({340.0f, 220.0f});
    this->m_buttonMenu->addChild(plusBtn);

    // 2. Noclip & Trajectory
    auto noclipToggle = CCMenuItemToggler::createWithStandardSprites(
        this, menu_selector(HackMenuLayer::onToggleNoclip), 0.7f
    );
    noclipToggle->toggle(cfg.noclip);
    noclipToggle->setPosition({45.0f, 175.0f});
    this->m_buttonMenu->addChild(noclipToggle);

    auto noclipTitle = CCLabelBMFont::create("Noclip", "bigFont.fnt");
    noclipTitle->setScale(0.4f);
    noclipTitle->setAnchorPoint({0.0f, 0.5f});
    noclipTitle->setPosition({70.0f, 175.0f});
    menu->addChild(noclipTitle);

    auto trajToggle = CCMenuItemToggler::createWithStandardSprites(
        this, menu_selector(HackMenuLayer::onToggleTrajectory), 0.7f
    );
    trajToggle->toggle(cfg.showTrajectory);
    trajToggle->setPosition({210.0f, 175.0f});
    this->m_buttonMenu->addChild(trajToggle);

    auto trajTitle = CCLabelBMFont::create("Trajectory", "bigFont.fnt");
    trajTitle->setScale(0.4f);
    trajTitle->setAnchorPoint({0.0f, 0.5f});
    trajTitle->setPosition({235.0f, 175.0f});
    menu->addChild(trajTitle);

    // 3. Speedhack
    auto speedToggle = CCMenuItemToggler::createWithStandardSprites(
        this, menu_selector(HackMenuLayer::onToggleSpeedhack), 0.7f
    );
    speedToggle->toggle(cfg.speedhackEnabled);
    speedToggle->setPosition({45.0f, 135.0f});
    this->m_buttonMenu->addChild(speedToggle);

    auto speedTitle = CCLabelBMFont::create("Speedhack", "bigFont.fnt");
    speedTitle->setScale(0.4f);
    speedTitle->setAnchorPoint({0.0f, 0.5f});
    speedTitle->setPosition({70.0f, 135.0f});
    menu->addChild(speedTitle);

    auto speedMinusSpr = ButtonSprite::create("-", "goldFont.fnt", "GJ_button_01.png", 0.6f);
    speedMinusSpr->setScale(0.7f);
    auto speedMinus = CCMenuItemSpriteExtra::create(
        speedMinusSpr, this, menu_selector(HackMenuLayer::onSpeedMinus)
    );
    speedMinus->setPosition({230.0f, 135.0f});
    this->m_buttonMenu->addChild(speedMinus);

    m_speedLabel = CCLabelBMFont::create(
        fmt::format("{:.1f}x", cfg.speedhackValue).c_str(), "bigFont.fnt"
    );
    m_speedLabel->setScale(0.35f);
    m_speedLabel->setPosition({285.0f, 135.0f});
    menu->addChild(m_speedLabel);

    auto speedPlusSpr = ButtonSprite::create("+", "goldFont.fnt", "GJ_button_01.png", 0.6f);
    speedPlusSpr->setScale(0.7f);
    auto speedPlus = CCMenuItemSpriteExtra::create(
        speedPlusSpr, this, menu_selector(HackMenuLayer::onSpeedPlus)
    );
    speedPlus->setPosition({340.0f, 135.0f});
    this->m_buttonMenu->addChild(speedPlus);

    // 4. Practice Music Hack
    auto musicToggle = CCMenuItemToggler::createWithStandardSprites(
        this, menu_selector(HackMenuLayer::onTogglePracticeMusic), 0.7f
    );
    musicToggle->toggle(cfg.practiceMusic);
    musicToggle->setPosition({45.0f, 95.0f});
    this->m_buttonMenu->addChild(musicToggle);

    auto musicTitle = CCLabelBMFont::create("Practice Music", "bigFont.fnt");
    musicTitle->setScale(0.4f);
    musicTitle->setAnchorPoint({0.0f, 0.5f});
    musicTitle->setPosition({70.0f, 95.0f});
    menu->addChild(musicTitle);

    // 5. Accurate Percentage
    auto percentToggle = CCMenuItemToggler::createWithStandardSprites(
        this, menu_selector(HackMenuLayer::onToggleAccuratePercent), 0.7f
    );
    percentToggle->toggle(cfg.accuratePercentage);
    percentToggle->setPosition({210.0f, 95.0f});
    this->m_buttonMenu->addChild(percentToggle);

    auto percentTitle = CCLabelBMFont::create("Accurate %", "bigFont.fnt");
    percentTitle->setScale(0.4f);
    percentTitle->setAnchorPoint({0.0f, 0.5f});
    percentTitle->setPosition({235.0f, 95.0f});
    menu->addChild(percentTitle);

    // 6. CPS Counter
    auto cpsToggle = CCMenuItemToggler::createWithStandardSprites(
        this, menu_selector(HackMenuLayer::onToggleCPS), 0.7f
    );
    cpsToggle->toggle(cfg.cpsCounter);
    cpsToggle->setPosition({45.0f, 55.0f});
    this->m_buttonMenu->addChild(cpsToggle);

    auto cpsTitle = CCLabelBMFont::create("CPS Counter", "bigFont.fnt");
    cpsTitle->setScale(0.4f);
    cpsTitle->setAnchorPoint({0.0f, 0.5f});
    cpsTitle->setPosition({70.0f, 55.0f});
    menu->addChild(cpsTitle);

    // 7. Safe Mode
    auto safeToggle = CCMenuItemToggler::createWithStandardSprites(
        this, menu_selector(HackMenuLayer::onToggleSafeMode), 0.7f
    );
    safeToggle->toggle(cfg.safeMode);
    safeToggle->setPosition({210.0f, 55.0f});
    this->m_buttonMenu->addChild(safeToggle);

    auto safeTitle = CCLabelBMFont::create("Safe Mode", "bigFont.fnt");
    safeTitle->setScale(0.4f);
    safeTitle->setAnchorPoint({0.0f, 0.5f});
    safeTitle->setPosition({235.0f, 55.0f});
    menu->addChild(safeTitle);

    return true;
}

void HackMenuLayer::onTogglePrediction(CCObject* sender) {
    auto& cfg = Config::get();
    cfg.predictionEnabled = !cfg.predictionEnabled;
    cfg.saveToSettings();
}

void HackMenuLayer::onWindowMinus(CCObject* sender) {
    auto& cfg = Config::get();
    if (cfg.predictionWindow > 1) {
        cfg.predictionWindow--;
        if (m_windowLabel) {
            m_windowLabel->setString(fmt::format("{} frames", cfg.predictionWindow).c_str());
        }
        cfg.saveToSettings();
    }
}

void HackMenuLayer::onWindowPlus(CCObject* sender) {
    auto& cfg = Config::get();
    if (cfg.predictionWindow < 30) {
        cfg.predictionWindow++;
        if (m_windowLabel) {
            m_windowLabel->setString(fmt::format("{} frames", cfg.predictionWindow).c_str());
        }
        cfg.saveToSettings();
    }
}

void HackMenuLayer::onToggleNoclip(CCObject* sender) {
    auto& cfg = Config::get();
    cfg.noclip = !cfg.noclip;
    cfg.saveToSettings();
}

void HackMenuLayer::onToggleTrajectory(CCObject* sender) {
    auto& cfg = Config::get();
    cfg.showTrajectory = !cfg.showTrajectory;
    cfg.saveToSettings();
}

void HackMenuLayer::onToggleSpeedhack(CCObject* sender) {
    auto& cfg = Config::get();
    cfg.speedhackEnabled = !cfg.speedhackEnabled;
    cfg.saveToSettings();
}

void HackMenuLayer::onSpeedMinus(CCObject* sender) {
    auto& cfg = Config::get();
    if (cfg.speedhackValue > 0.2f) {
        cfg.speedhackValue = std::max(0.1f, cfg.speedhackValue - 0.1f);
        if (m_speedLabel) {
            m_speedLabel->setString(fmt::format("{:.1f}x", cfg.speedhackValue).c_str());
        }
        cfg.saveToSettings();
    }
}

void HackMenuLayer::onSpeedPlus(CCObject* sender) {
    auto& cfg = Config::get();
    if (cfg.speedhackValue < 5.0f) {
        cfg.speedhackValue += 0.1f;
        if (m_speedLabel) {
            m_speedLabel->setString(fmt::format("{:.1f}x", cfg.speedhackValue).c_str());
        }
        cfg.saveToSettings();
    }
}

void HackMenuLayer::onTogglePracticeMusic(CCObject* sender) {
    auto& cfg = Config::get();
    cfg.practiceMusic = !cfg.practiceMusic;
    cfg.saveToSettings();
}

void HackMenuLayer::onToggleAccuratePercent(CCObject* sender) {
    auto& cfg = Config::get();
    cfg.accuratePercentage = !cfg.accuratePercentage;
    cfg.saveToSettings();
}

void HackMenuLayer::onToggleCPS(CCObject* sender) {
    auto& cfg = Config::get();
    cfg.cpsCounter = !cfg.cpsCounter;
    cfg.saveToSettings();
}

void HackMenuLayer::onToggleSafeMode(CCObject* sender) {
    auto& cfg = Config::get();
    cfg.safeMode = !cfg.safeMode;
    cfg.saveToSettings();
}
