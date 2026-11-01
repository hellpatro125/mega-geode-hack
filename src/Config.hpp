#pragma once

#include <Geode/Geode.hpp>

class Config {
public:
    static Config& get() {
        static Config instance;
        return instance;
    }

    // Prediction / Assist settings
    bool predictionEnabled = true;
    int predictionWindow = 5; // number of frames to look ahead / delay
    std::string predictionMode = "Hazard Avoidance & Orb Snapping";

    // Classic MegaHack hacks
    bool noclip = false;
    bool speedhackEnabled = false;
    float speedhackValue = 1.0f;
    bool practiceMusic = true;
    bool showHitboxes = false;
    bool accuratePercentage = true;
    bool cpsCounter = true;
    bool safeMode = true;

    // Load from Geode settings
    void loadFromSettings() {
        auto* mod = geode::Mod::get();
        if (!mod) return;

        predictionEnabled = mod->getSettingValue<bool>("prediction-enabled");
        predictionWindow = static_cast<int>(mod->getSettingValue<int64_t>("prediction-window"));
        predictionMode = mod->getSettingValue<std::string>("prediction-mode");
        noclip = mod->getSettingValue<bool>("noclip");
        speedhackEnabled = mod->getSettingValue<bool>("speedhack-enabled");
        speedhackValue = static_cast<float>(mod->getSettingValue<double>("speedhack-value"));
        practiceMusic = mod->getSettingValue<bool>("practice-music");
        showHitboxes = mod->getSettingValue<bool>("show-hitboxes");
        accuratePercentage = mod->getSettingValue<bool>("accurate-percentage");
        cpsCounter = mod->getSettingValue<bool>("cps-counter");
        safeMode = mod->getSettingValue<bool>("safe-mode");
    }

    void saveToSettings() {
        auto* mod = geode::Mod::get();
        if (!mod) return;

        (void)mod->setSettingValue<bool>("prediction-enabled", predictionEnabled);
        (void)mod->setSettingValue<int64_t>("prediction-window", predictionWindow);
        (void)mod->setSettingValue<std::string>("prediction-mode", predictionMode);
        (void)mod->setSettingValue<bool>("noclip", noclip);
        (void)mod->setSettingValue<bool>("speedhack-enabled", speedhackEnabled);
        (void)mod->setSettingValue<double>("speedhack-value", speedhackValue);
        (void)mod->setSettingValue<bool>("practice-music", practiceMusic);
        (void)mod->setSettingValue<bool>("show-hitboxes", showHitboxes);
        (void)mod->setSettingValue<bool>("accurate-percentage", accuratePercentage);
        (void)mod->setSettingValue<bool>("cps-counter", cpsCounter);
        (void)mod->setSettingValue<bool>("safe-mode", safeMode);
    }
};
