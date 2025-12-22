#include <Windows.h>
#include <iostream>
#include <timeapi.h>

enum DisplayModes {
    Windowed,
    Borderless,
    Fullscreen
};

struct RatataRConfig {
    unsigned int width;
    unsigned int height;
    float fov;
    float climbFOV;
    double runSlideFOV;
    bool console;
    bool popupMenu;
    bool invertVerticalLook;
    bool removeFpsCap;
    bool autoSave;
    bool improvedViewDistance;
    bool fog;
    bool noBonks;
    bool speedrunMode;
    DisplayModes displayMode;
};

DisplayModes parseDisplayMode(const char* value) {
    if (strcmp(value, "WINDOWED") == 0) return Windowed;
    if (strcmp(value, "FULLSCREEN") == 0) return Fullscreen;
    return Borderless;
}

struct IntOption {
    const char* key;
    unsigned int RatataRConfig::* member;
    int def;
};

struct BoolOption {
    const char* key;
    bool RatataRConfig::* member;
    bool def;
};

struct FloatOption {
    const char* key;
    float RatataRConfig::* member;
    int def;
};

void loadConfig(RatataRConfig& cfg, const std::string& configPath) {
    const char* sectionName = "CONFIG";

    IntOption intOptions[] = {
        {"width",                       &RatataRConfig::width,                              0},
        {"height",                      &RatataRConfig::height,                             0},
    };

    BoolOption boolOptions[] = {
        {"console",                     &RatataRConfig::console,                       false},
        {"popupMenu",                   &RatataRConfig::popupMenu,                     false},
        {"invertVerticalLook",          &RatataRConfig::invertVerticalLook,            false},
        {"removeFpsCap",                &RatataRConfig::removeFpsCap,                  false},
        {"autoSave",                    &RatataRConfig::autoSave,                      true},
        {"improvedViewDistance",        &RatataRConfig::improvedViewDistance,          false},
        {"fog",                         &RatataRConfig::fog,                           true},
        {"noBonks",                     &RatataRConfig::noBonks,                       false},
        {"speedrunMode",                &RatataRConfig::speedrunMode,                  false},
    };
    
    FloatOption floatOptions[] = {
        {"fov",                         &RatataRConfig::fov,                                95},
    };

    // Get int options
    for (const auto& opt : intOptions) {
        cfg.*(opt.member) = GetPrivateProfileIntA(
            sectionName,
            opt.key,
            opt.def,
            configPath.c_str()
        );
    }

    // Get boolean options
    for (const auto& opt : boolOptions) {
        cfg.*(opt.member) = GetPrivateProfileIntA(
            sectionName,
            opt.key,
            opt.def ? 1 : 0,
            configPath.c_str()
        ) != 0;
    }

    // Get float options
    for (const auto& opt : floatOptions) {
        cfg.*(opt.member) = static_cast<float>(
            GetPrivateProfileIntA(
                sectionName,
                opt.key,
                opt.def,
                configPath.c_str()
            )
        );
    }

    // Get display mode
    char displayModeBuffer[32];
    GetPrivateProfileStringA(
        sectionName,
        "displayMode",
        "BORDERLESS",
        displayModeBuffer,
        sizeof(displayModeBuffer),
        configPath.c_str()
    );

    for (size_t i = 0; displayModeBuffer[i] != '\0'; i++) {
        displayModeBuffer[i] = static_cast<char>(
            std::toupper(static_cast<unsigned char>(displayModeBuffer[i]))
        );
    }

    cfg.displayMode = parseDisplayMode(displayModeBuffer);

    // Clamp FOV between 1 and 155
    if (cfg.fov < 1.0f) cfg.fov = 1.0f;
    else if (cfg.fov > 155.0f) cfg.fov = 155.0f;
    
    // Update dependent values
    cfg.climbFOV = (110.0f / 95.0f) * cfg.fov;
    cfg.runSlideFOV = (110.0 / 95.0) * cfg.fov;

    // Set to screen resolution if width is 0
    if (cfg.width == 0) {
        cfg.width = GetSystemMetrics(SM_CXSCREEN);
    }

    // Set to screen resolution if height is 0
    if (cfg.height == 0) {
        cfg.height = GetSystemMetrics(SM_CYSCREEN);
    }
}

std::string rootDirectory;

bool hook(void *toHook, void *ourFunc, size_t len) {
    if (len < 5) {
        return false;
    }

    DWORD curProtection;
    VirtualProtect(toHook, len, PAGE_EXECUTE_READWRITE, &curProtection);

    memset(toHook,0x90,len);

    DWORD relativeAddress = ((DWORD)ourFunc - (DWORD)toHook) - 5;

    *(BYTE*)toHook = 0xE9;
    *(DWORD*)((DWORD)toHook+1) = relativeAddress;

    VirtualProtect(toHook, len, curProtection, &curProtection);

    return true;
}

DWORD jmpBackAddressCursor;
void __declspec(naked) hClipCursor() {
    __asm {
        call GetClientRect
        lea ecx, [esp+0x1c]
        mov edx, dword ptr ds:[0x007df95c]
        push 0x2
        push ecx
        push 0x0
        push edx
        call MapWindowPoints
        jmp [jmpBackAddressCursor]
    }
}

DWORD jmpBackAddressSetWindowPosPush;
void __declspec(naked) hSetWindowPosPushBL() {
    __asm {
        push 0x0
        sub eax,0x14
        sub ebp,0x6
        push eax
        push ebp
        jmp [jmpBackAddressSetWindowPosPush]
    }
}

void __declspec(naked) hSetWindowPosPushWND() {
    __asm {
        push 0x0
        add eax,0x9
        push eax
        push ebp
        jmp [jmpBackAddressSetWindowPosPush]
    }
}

DWORD jmpBackAddressConsoleEnable;
void __declspec(naked) hEnableConsole() {
    __asm {
        or dword ptr ds:[ecx+0x6C6C],eax
        test byte ptr ds:[ecx+0x6C6C],0x40
        jz go_back
        mov eax,dword ptr ds:[ecx+0x6C8C]
        test eax,eax
        jz go_back
        push 0x1
        push eax
        call ShowWindow
        mov eax,dword ptr ss:[esp+4]
        go_back:
        jmp [jmpBackAddressConsoleEnable]
    }
}


DWORD jmpBackAddressShowConsole;
void __declspec(naked) hShowConsole() {
    __asm {
        and dword ptr ds:[esi+0x6C6C],ebx
        test byte ptr ds:[esi+0x6C6C],0x40
        jnz go_back
        mov ebx,dword ptr ds:[esi+0x6C8C]
        test ebx,ebx
        jz go_back
        push 0x0
        push ebx
        call ShowWindow
        go_back:
        jmp [jmpBackAddressShowConsole]
    }
}

DWORD jmpBackAddressFpsFix1;
void __declspec(naked) hFpsFix1() {
    __asm {
        add dword ptr ds:[0x007df954],0x1
        push 0x1
        call timeBeginPeriod
        jmp [jmpBackAddressFpsFix1]
    }
}

DWORD jmpBackAddressFpsFix2;
uint32_t cleanUp = 0x005c5170;
void __declspec(naked) hFpsFix2() {
    __asm {
        call cleanUp
        push 0x1
        call timeEndPeriod
        jmp [jmpBackAddressFpsFix2]
    }
}

void patch(BYTE* ptr, BYTE* buf, size_t len) {
    DWORD curProtection;
    VirtualProtect(ptr, len, PAGE_EXECUTE_READWRITE, &curProtection);
    while (len > 0) {
        *(BYTE*)ptr = *buf;
        ptr++;
        buf++;
        len--;
    }
    VirtualProtect(ptr, len, curProtection, &curProtection);
}

void applyBasePatches(RatataRConfig& cfg) {
    BYTE zero[] = {0x00,0x00,0x00,0x00};
    BYTE jmp[] = {0xEB};
    BYTE nop[] = {0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90};
    BYTE patchCursorHide[] = {0x66,0x81,0x7c,0x24,0x18,0x01,0x00,0x75};
    BYTE patchWindowed[] = {0xfe,0x85};
    BYTE patchWindowPosBorder[] = {0xfe,0xff,0xff,0xff};
    BYTE patchBorderless[] = {0x0a};
    BYTE patchWindowKey[] = {0x06};

    //Cursor only gets hidden when inside client area
    patch((BYTE*)0x654711, patchCursorHide, 8);
    //Setting custom res
    if (cfg.displayMode != DisplayModes::Fullscreen) {
        patch((BYTE*)0x657e58, zero, 1);
        patch((BYTE*)0x657e5e, reinterpret_cast<BYTE*>(&cfg.width), 2);
        patch((BYTE*)0x657e65, reinterpret_cast<BYTE*>(&cfg.height), 2);
    }
    //Initial window position
    patch((BYTE*)0x658d05, zero, 1);
    //Not showing window before d3d9 device is created
    patch((BYTE*)0x658d0a, zero, 1);
    patch((BYTE*)0x658d00, zero, 1);
    //Make game run windowed
    if (cfg.displayMode != DisplayModes::Fullscreen) {
        patch((BYTE*)0x674750, patchWindowed, 2);
    }
    //Set window position to top left corner
    patch((BYTE*)0x67a9d9, cfg.displayMode == DisplayModes::Borderless ? zero : patchWindowPosBorder, 4);
    if (cfg.displayMode == DisplayModes::Borderless) {
        patch((BYTE*)0x67a9ef, patchBorderless, 1);
    }
    //Change directinput flag so windows key works
    patch((BYTE*)0x6a91c6, patchWindowKey, 1);

    //Bypass disc requirement
    patch((BYTE*)0x00654F52, nop, 6);
    patch((BYTE*)0x00654FD7, nop, 2);
    patch((BYTE*)0x00655015, jmp, 1);

    // Tell the game where the disc files can be found
    const size_t len = rootDirectory.length();
    BYTE* noCDDirectory = new BYTE[len + 1];

    for (size_t i = 0; i < len; ++i) {
        noCDDirectory[i] = static_cast <BYTE>(rootDirectory[i]);
    }
    noCDDirectory[len] = '\0';
    patch((BYTE*)0x007DF135, noCDDirectory, len);

    delete[] noCDDirectory;

    // Stop directory from being overwritten
    // This one is scary because I don't exactly know what the purpose if this one is.
    // Please check this when you have the time.
    patch((BYTE*)0x00654D8D, nop, 6);

    //Allow banned save names
    patch((BYTE*)0x004E8F5A, jmp, 1);

    //Allow entering blank save name
    patch((BYTE*)0x00557AAA, jmp, 1);

    //Allow multiple game instances
    patch((BYTE*)0x00656367, jmp, 1);
    patch((BYTE*)0x00656386, nop, 2);

    if (cfg.invertVerticalLook) {
        patch((BYTE*)0x00406DB4, (BYTE*)"\x75", 1);
    }

    if (!cfg.autoSave) {
        patch((BYTE*)0x005599F8, nop, 13);
    }
}

void applyNonSpeedrunPatches(RatataRConfig& cfg) {
    // These patches will not get applied when speedrun mode is turned on!

    //Apply FOV
    patch((BYTE*)0x00580C7C, (BYTE*)"\xEB", 1); //BYPASS FOV OVERFLOW ERROR
    patch((BYTE*)0x0070A7D8, reinterpret_cast<BYTE*>(&cfg.fov), sizeof(float));
    patch((BYTE*)0x0070A7A4, reinterpret_cast<BYTE*>(&cfg.climbFOV), sizeof(float));
    patch((BYTE*)0x0070A798, reinterpret_cast<BYTE*>(&cfg.runSlideFOV), sizeof(double));

    if (cfg.improvedViewDistance) {
        //Use default far value
        patch((BYTE*)0x00402326, (BYTE*)"\x90\x90", 2);
        patch((BYTE*)0x00402337, (BYTE*)"\x90\x90", 2);
        //Set default far value to 500
        patch((BYTE*)0x0070A5B0, (BYTE*)"\x00\x00\xFA\x43", 4);

        //Remove culling
        patch((BYTE*)0x00678FD3, (BYTE*)"\xEB", 1);
        patch((BYTE*)0x00678FB1, (BYTE*)"\x90\x90\x90\x90\x90\x90", 6);
        patch((BYTE*)0x006175D2, (BYTE*)"\x90\x90", 2);
        patch((BYTE*)0x006175DF, (BYTE*)"\x90\x90", 2);
        patch((BYTE*)0x006EDB57, (BYTE*)"\x90\x90", 2);
        patch((BYTE*)0x006EDB61, (BYTE*)"\x90\x90", 2);

        //Force max Lod
        patch((BYTE*)0x00617655, (BYTE*)"\xEB", 1);

        //Remove item freezing
        patch((BYTE*)0x0049CA91, (BYTE*)"\xEB", 1);
    }

    if (!cfg.fog) {
        patch((BYTE*)0x00678593, (BYTE*)"\xE9\x8F\x00\x00\x00\x90", 6);
    }

    if (cfg.noBonks) {
        patch((BYTE*)0x0043dd76, (BYTE*)"\xE9\x16\x01\x00\x00\x90", 6);
    }

    //Enable console
    if (cfg.console) {
        patch((BYTE*)0x0066A2E4, (BYTE*)"\x90\x90\x90", 3);
    }

    if (cfg.popupMenu) {
        patch((BYTE*)0x00670983, (BYTE*)"\x90\x90", 2);
        patch((BYTE*)0x00674738, (BYTE*)"\x90\x90\x90\x90\x90\x90", 6);
    }

    if (cfg.removeFpsCap) {
        patch((BYTE*)0x006589D7, (BYTE*)"\x90\x90\x90\x90\x90\x90\x90\x90", 8);
    }
}

DWORD WINAPI MainThread(LPVOID param) {
    TCHAR moduleFileName[MAX_PATH];
    GetModuleFileNameA((HMODULE)param, (LPSTR)moduleFileName, MAX_PATH);
    std::string::size_type pos = std::string((char*)moduleFileName).find_last_of("\\/");
    std::string configPath = std::string((char*)moduleFileName).substr(0, pos).append("\\").append("RatataRconfig.ini");
    rootDirectory = std::string((char*)moduleFileName).substr(0, pos);

    DWORD hookAddressCursor = 0x670991;
    DWORD hookAddressSetWindowPosPush = 0x67a9d0;
    DWORD hookAddressConsoleEnable = 0x66a2e7;
    DWORD hookAddressShowConsole = 0x005fbd0b;
    DWORD hookAddressFpsFix1 = 0x005c6914;
    DWORD hookAddressFpsFix2 = 0x005c6932;
    size_t hookLengthCursor = 6;
    size_t hookLengthSetWindowPosPush = 6;
    size_t hookLengthConsoleEnable = 6;
    size_t hookLengthShowConsole = 6;
    size_t hookLengthFpsFix1 = 7;
    size_t hookLengthFpsFix2 = 5;
    jmpBackAddressCursor = hookAddressCursor+hookLengthCursor;
    jmpBackAddressSetWindowPosPush = hookAddressSetWindowPosPush+hookLengthSetWindowPosPush;
    jmpBackAddressConsoleEnable = hookAddressConsoleEnable+hookLengthConsoleEnable;
    jmpBackAddressShowConsole = hookAddressShowConsole+hookLengthShowConsole;
    jmpBackAddressFpsFix1 = hookAddressFpsFix1+hookLengthFpsFix1;
    jmpBackAddressFpsFix2 = hookAddressFpsFix2+hookLengthFpsFix2;

    RatataRConfig config;
    loadConfig(config, configPath);

    applyBasePatches(config);
    if (!config.speedrunMode) {
        applyNonSpeedrunPatches(config);
    }

    hook((void*)hookAddressCursor,hClipCursor,hookLengthCursor);
    if (config.displayMode == DisplayModes::Borderless) {
        hook((void*)hookAddressSetWindowPosPush, hSetWindowPosPushBL, hookLengthSetWindowPosPush);
    }

    else if (config.displayMode != DisplayModes::Fullscreen) {
        hook((void*)hookAddressSetWindowPosPush, hSetWindowPosPushWND, hookLengthSetWindowPosPush);
    }

    if (config.console) {
        hook((void*)hookAddressConsoleEnable, hEnableConsole, hookLengthConsoleEnable);
        hook((void*)hookAddressShowConsole, hShowConsole, hookLengthShowConsole);
    }

    hook((void*)hookAddressFpsFix1,hFpsFix1,hookLengthFpsFix1);
    hook((void*)hookAddressFpsFix2,hFpsFix2,hookLengthFpsFix2);
    return 0;
}

BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
            CreateThread(0,0,MainThread,hModule,0,0);
            break;
    }
    return true;
}
