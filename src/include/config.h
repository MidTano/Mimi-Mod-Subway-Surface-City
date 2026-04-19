#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace Config {

struct Settings {
    bool menuVisible = false;
    bool godModeEnabled = false;
    float gameSpeed = 1.0f;
    bool infiniteCoins = false;
    bool infiniteKeys = false;
    bool noClip = false;
    float fov = 90.0f;
    int menuStyle = 0;
};

class ConfigManager {
public:
    static ConfigManager& getInstance();

    bool load(const std::string& path);
    bool save(const std::string& path);

    Settings& getSettings();
    const Settings& getSettings() const;

private:
    ConfigManager();
    ~ConfigManager();

    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    Settings settings;
};

}
