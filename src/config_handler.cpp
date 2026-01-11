#include "config_handler.h"
#include <Windows.h>
#include <string>
#include <cctype>

void ConfigHandler::load(const std::string& path) {
    configPath = path;
    constexpr char section[] = "CONFIG";
    
    // Integer options
    LoadOption(section, "width", &RatataRConfig::width);
    LoadOption(section, "height", &RatataRConfig::height);
    LoadOption(section, "maxFps", &RatataRConfig::maxFps);
    LoadOption(section, "fov", &RatataRConfig::fov);

    // Clamp FOV
    constexpr unsigned int fovMin = 1;
    constexpr unsigned int fovMax = 155;
    if (cfg.fov < fovMin) cfg.fov = fovMin;
    else if (cfg.fov > fovMax) cfg.fov = fovMax;

    // Boolean options
    LoadOption(section, "console", &RatataRConfig::console);
    LoadOption(section, "popupMenu", &RatataRConfig::popupMenu);
    LoadOption(section, "invertVerticalLook", &RatataRConfig::invertVerticalLook);
    LoadOption(section, "removeFpsCap", &RatataRConfig::removeFpsCap);
    LoadOption(section, "autoSave", &RatataRConfig::autoSave);
    LoadOption(section, "improvedViewDistance", &RatataRConfig::improvedViewDistance);
    LoadOption(section, "fog", &RatataRConfig::fog);
    LoadOption(section, "noBonks", &RatataRConfig::noBonks);
    LoadOption(section, "speedrunMode", &RatataRConfig::speedrunMode);
    LoadOption(section, "discordRichPresence", &RatataRConfig::discordRichPresence);
    LoadOption(section, "displayFrameCounter", &RatataRConfig::displayFrameCounter);

    // Get display mode
    char displayModeBuffer[32];
    GetPrivateProfileStringA(
        section,
        "displayMode",
        "BORDERLESS",
        displayModeBuffer,
        sizeof(displayModeBuffer),
        path.c_str()
    );
    cfg.displayMode = ParseDisplayMode(displayModeBuffer);
}

void ConfigHandler::save(const std::string& path) const {
    constexpr char section[] = "CONFIG";

    // Int
    SaveOption(section, "width", cfg.width);
    SaveOption(section, "height", cfg.height);
    SaveOption(section, "maxFps", cfg.maxFps);
    SaveOption(section, "fov", cfg.fov);

    // Bool
    SaveOption(section, "console", cfg.console);
    SaveOption(section, "popupMenu", cfg.popupMenu);
    SaveOption(section, "invertVerticalLook", cfg.invertVerticalLook);
    SaveOption(section, "removeFpsCap", cfg.removeFpsCap);
    SaveOption(section, "autoSave", cfg.autoSave);
    SaveOption(section, "improvedViewDistance", cfg.improvedViewDistance);
    SaveOption(section, "fog", cfg.fog);
    SaveOption(section, "noBonks", cfg.noBonks);
    SaveOption(section, "speedrunMode", cfg.speedrunMode);
    SaveOption(section, "discordRichPresence", cfg.discordRichPresence);
    SaveOption(section, "displayFrameCounter", cfg.displayFrameCounter);

    std::string modeStr;
    switch (cfg.displayMode) {
    case DisplayModes::Windowed: modeStr = "Windowed"; break;
    case DisplayModes::Fullscreen: modeStr = "Fullscreen"; break;
    default: modeStr = "Borderless";
    }

    WritePrivateProfileStringA(section, "displayMode", modeStr.c_str(), path.c_str());
}

const RatataRConfig& ConfigHandler::getConfig() const { return cfg; }
RatataRConfig& ConfigHandler::getConfig() { return cfg; }

RemyFOV getFOV(RatataRConfig& cfg) {
    constexpr float CLIMBING_RATIO = 110.0f / 95.0f;
    constexpr double RUNNING_SLIDING_RATIO = 110.0 / 95.0;

    float userFOV = static_cast<float>(cfg.fov);
    RemyFOV g_Fov;
    g_Fov.normal = userFOV;
    g_Fov.climbing = CLIMBING_RATIO * userFOV;
    g_Fov.runningSliding = static_cast<float>(RUNNING_SLIDING_RATIO * userFOV);
    return g_Fov;
}

DisplayModes ConfigHandler::ParseDisplayMode(const char* value) {
    std::string upper;
    for (size_t i = 0; value[i] != '\0'; i++) {
        upper += static_cast<char>(std::toupper(static_cast<unsigned char>(value[i])));
    }

    if (upper == "WINDOWED") return DisplayModes::Windowed;
    if (upper == "FULLSCREEN") return DisplayModes::Fullscreen;
    return DisplayModes::Borderless;
}