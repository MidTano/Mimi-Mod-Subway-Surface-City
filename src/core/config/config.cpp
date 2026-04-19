#include "config.h"

#include <fstream>

namespace Config {

ConfigManager::ConfigManager() {
}

ConfigManager::~ConfigManager() {
}

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

bool ConfigManager::load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    try {
        nlohmann::json j;
        file >> j;

        if (j.contains("menuVisible")) settings.menuVisible = j["menuVisible"];
        if (j.contains("godModeEnabled")) settings.godModeEnabled = j["godModeEnabled"];
        if (j.contains("gameSpeed")) settings.gameSpeed = j["gameSpeed"];
        if (j.contains("infiniteCoins")) settings.infiniteCoins = j["infiniteCoins"];
        if (j.contains("infiniteKeys")) settings.infiniteKeys = j["infiniteKeys"];
        if (j.contains("noClip")) settings.noClip = j["noClip"];
        if (j.contains("fov")) settings.fov = j["fov"];
        if (j.contains("menuStyle")) settings.menuStyle = j["menuStyle"];

        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

bool ConfigManager::save(const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }

    try {
        nlohmann::json j;
        j["menuVisible"] = settings.menuVisible;
        j["godModeEnabled"] = settings.godModeEnabled;
        j["gameSpeed"] = settings.gameSpeed;
        j["infiniteCoins"] = settings.infiniteCoins;
        j["infiniteKeys"] = settings.infiniteKeys;
        j["noClip"] = settings.noClip;
        j["fov"] = settings.fov;
        j["menuStyle"] = settings.menuStyle;

        file << j.dump(4);
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

Settings& ConfigManager::getSettings() {
    return settings;
}

const Settings& ConfigManager::getSettings() const {
    return settings;
}

}
