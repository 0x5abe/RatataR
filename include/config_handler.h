#pragma once
#include <string>

enum class DisplayModes {
    Windowed,
    Borderless,
    Fullscreen
};

struct RatataRConfig {
    unsigned int width = 0;
    unsigned int height = 0;
    unsigned int maxFps = 0;
    unsigned int fov = 95;
    bool console = false;
    bool popupMenu = false;
    bool invertVerticalLook = false;
    bool removeFpsCap = false;
    bool autoSave = true;
    bool improvedViewDistance = false;
    bool fog = true;
    bool noBonks = false;
    bool speedrunMode = false;
    bool discordRichPresence = false;
    bool displayFrameCounter = false;
    DisplayModes displayMode = DisplayModes::Borderless;
};

struct RemyFOV {
    float normal = 95.0f;
    float climbing = 110.0f;
    double runningSliding = 110.0;
};

RemyFOV getFOV(RatataRConfig& cfg);

class ConfigHandler {
public:
    ConfigHandler() = default;

    void load(const std::string& path);
    void save(const std::string& path) const;

    const RatataRConfig& getConfig() const;
    RatataRConfig& getConfig();

private:
    RatataRConfig cfg;
    std::string configPath;

    template<typename T>
    void LoadOption(const char* section, const char* key, T RatataRConfig::* member) {
        if constexpr (std::is_same_v<T, bool>) {
            cfg.*member = GetPrivateProfileIntA(section, key, cfg.*member ? 1 : 0, configPath.c_str()) != 0;
        }
        else if constexpr (std::is_integral_v<T>) {
            cfg.*member = GetPrivateProfileIntA(section, key, cfg.*member, configPath.c_str());
        }
    }

    template<typename T>
    void SaveOption(const char* section, const char* key, T value) const {
        char buffer[32];
        if constexpr (std::is_same_v<T, bool> || std::is_integral_v<T>) {
            snprintf(buffer, sizeof(buffer), "%d", static_cast<int>(value));
            WritePrivateProfileStringA(section, key, buffer, configPath.c_str());
        }
    }

    static DisplayModes ParseDisplayMode(const char* value);
};