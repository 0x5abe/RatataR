#include <Windows.h>
#include <iostream>
#include <timeapi.h>
#include <Psapi.h>
#include <vector>
#include <sstream>

struct PatchAddresses
{
    uintptr_t hookAddressCursor;
    uintptr_t hClipCursor;

    uintptr_t hookAddressSetWindowPosPush;
    uintptr_t hookAddressConsoleEnable;
    uintptr_t hookAddressShowConsole;

    uintptr_t hookAddressFpsFix1;
    uintptr_t hookAddressFpsFix2;
    uintptr_t hFpsFix1;
    uintptr_t hFpsFix2CleanUp;

    uintptr_t patchCursorHide;
    std::vector<BYTE> patchCursorHidePatch;

    uintptr_t customResPatch1;
    uintptr_t customResPatch2;
    uintptr_t customResPatch3;

    uintptr_t initialWindowPositionPatch;

    uintptr_t notShowingWindowBeforeD3d9DeviceCreatedPatch1;
    uintptr_t notShowingWindowBeforeD3d9DeviceCreatedPatch2;

    uintptr_t patchWindowed;
    uintptr_t patchWindowPosBorderlessOrBorder;
    uintptr_t patchBorderless;
    uintptr_t patchWindowKey;

    uintptr_t bypassDiscRequirementPatch1;
    uintptr_t bypassDiscRequirementPatch2;
    uintptr_t bypassDiscRequirementPatch3;

    uintptr_t musicVideoDirectory;
    uintptr_t stopMusicVideoDirectoryOverwrite;

    uintptr_t bypassFovOverflow;

    uintptr_t fov;
    uintptr_t climbFov;
    uintptr_t runSlideFov;

    uintptr_t patchConsole;

    uintptr_t patchPopupMenu0;
    uintptr_t patchPopupMenu1;

    uintptr_t patchRemoveFpsCap;
    uintptr_t patchInvertVerticalLook;

    uintptr_t patchAutoSave;

    uintptr_t patchAllowEmptySaveNames;
    uintptr_t patchAllowBannedSaveNames;

    uintptr_t patchAllowMultiInstances1;
    uintptr_t PatchAllowMultiInstances2;

    uintptr_t patchFog;

    uintptr_t patchNoBonks;

    uintptr_t patchForceMaxLod;

    uintptr_t patchRemoveItemFreezing;

    uintptr_t patchRemoveCulling1;
    uintptr_t patchRemoveCulling2;
    uintptr_t patchRemoveCulling3;
    uintptr_t patchRemoveCulling4;
    uintptr_t patchRemoveCulling5;
    uintptr_t patchRemoveCulling6;

    uintptr_t patchDefaultFarValue1;
    uintptr_t patchDefaultFarValue2;

    uintptr_t defaultFarValue;
};

std::string rootDirectory;
PatchAddresses addresses{};

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
    int def; // Will be read as int from config
};

void loadConfig(RatataRConfig& cfg, const std::string& configPath) {
    const char* sectionName = "CONFIG";

    IntOption intOptions[] = {
        {"width",                       &RatataRConfig::width,                            0},
        {"height",                      &RatataRConfig::height,                           0},
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
        {"fov",                         &RatataRConfig::fov,                              95},
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
    const float fovMin = 1.0f;
    const float fovMax = 155.0f;
    if (cfg.fov < fovMin) cfg.fov = fovMin;
    else if (cfg.fov > fovMax) cfg.fov = fovMax;
    
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

struct ModuleInfo {
    uintptr_t base;
    size_t size;
};

uintptr_t ReadPtr32(uintptr_t address)
{
    return *reinterpret_cast<uintptr_t*>(address);
}

std::vector<int> PatternToBytes(const std::string& pattern)
{
    std::vector<int> bytes;
    std::istringstream stream(pattern);
    std::string byte;

    while (stream >> byte) {
        if (byte == "??")
            bytes.push_back(-1);
        else
            bytes.push_back(std::stoi(byte, nullptr, 16));
    }

    return bytes;
}

ModuleInfo GetMainModule()
{
    HMODULE overlay = GetModuleHandle(nullptr);

    MODULEINFO info{};
    GetModuleInformation(GetCurrentProcess(), overlay, &info, sizeof(info));

    return {
        (uintptr_t)info.lpBaseOfDll,
        (size_t)info.SizeOfImage
    };
}

uintptr_t FindSignature(uintptr_t base, size_t size, const std::string& pattern)
{
    auto bytes = PatternToBytes(pattern);
    size_t patternSize = bytes.size();

    for (size_t i = 0; i <= size - patternSize; i++) {
        bool found = true;

        for (size_t j = 0; j < patternSize; j++) {
            if (bytes[j] != -1 && bytes[j] != *(uint8_t*)(base + i + j)) {
                found = false;
                break;
            }
        }

        if (found)
            return base + i;
    }

    return 0;
}

void getSignatures(PatchAddresses& address)
{
    auto mainMod = GetMainModule();
    const uintptr_t base = mainMod.base;
    const size_t size = mainMod.size;

    uintptr_t ptr = 0;

    ptr = FindSignature(base, size, "8B 15 ?? ?? ?? ?? 8D 4C 24");
    address.hookAddressCursor = ptr + 0x0C;
    address.hClipCursor = ReadPtr32(ptr + 0x2);

    ptr = FindSignature(base, size, "6A ?? 03 C7 50 55");
    address.hookAddressSetWindowPosPush = ptr;

    ptr = FindSignature(base, size, "09 81 ?? ?? ?? ?? C2 ?? ?? 81 EC");
    address.hookAddressConsoleEnable = ptr;

    ptr = FindSignature(base, size, "21 9E ?? ?? ?? ?? 5E");
    address.hookAddressShowConsole = ptr;

    ptr = FindSignature(base, size, "83 05 ?? ?? ?? ?? ?? 80 3D");
    address.hookAddressFpsFix1 = ptr;
    address.hookAddressFpsFix2 = ptr + 0x1E;
    address.hFpsFix1 = ReadPtr32(ptr + 0x2);
    
    address.hFpsFix2CleanUp = FindSignature(base, size, "53 56 57 E8 ?? ?? ?? ?? 8B 0D");

    ptr = FindSignature(base, size, "F6 05 ?? ?? ?? ?? ?? 0F 84 ?? ?? ?? ?? 6A"); // US
    if (ptr) {
        address.patchCursorHide = ptr;
        address.patchCursorHidePatch = { 0x66,0x81,0x7C,0x24,0x18,0x01,0x00,0x0F,0x85 };
    }
    else {
        ptr = FindSignature(base, size, "F6 05 ?? ?? ?? ?? ?? 74 ?? 6A"); // Other versions
        address.patchCursorHide = ptr;
        address.patchCursorHidePatch = { 0x66,0x81,0x7C,0x24,0x18,0x01,0x00,0x75 };
    }

    ptr = FindSignature(base, size, "0E 66 C7 44 24");
    address.customResPatch1 = ptr;
    address.customResPatch2 = ptr + 0x6;
    address.customResPatch3 = ptr + 0x0D;

    ptr = FindSignature(base, size, "80 68 ?? ?? C2");
    address.initialWindowPositionPatch = ptr;

    ptr = FindSignature(base, size, "80 68 ?? ?? 00 80");
    address.notShowingWindowBeforeD3d9DeviceCreatedPatch1 = ptr + 0xA;
    address.notShowingWindowBeforeD3d9DeviceCreatedPatch2 = ptr;

    ptr = FindSignature(base, size, "89 8D ?? ?? ?? ?? 89 0E");
    address.patchWindowed = ptr;

    ptr = FindSignature(base, size, "AE 01 00 00 6A");
    address.patchWindowPosBorderlessOrBorder = ptr;

    ptr = FindSignature(base, size, "CA ?? ?? F0 ?? FF 15");
    address.patchBorderless = ptr;

    ptr = FindSignature(base, size, "16 51 50");
    address.patchWindowKey = ptr;

    ptr = FindSignature(base, size, "0F 85 ?? ?? ?? ?? 8D 84 24 ?? ?? ?? ?? 8B D6");
    address.bypassDiscRequirementPatch1 = ptr;

    ptr = FindSignature(base, size, "74 ?? 53 53 8D 44 24");
    address.bypassDiscRequirementPatch2 = ptr;
    address.bypassDiscRequirementPatch3 = ptr + 0x3E;

    ptr = FindSignature(base, size, "B8 ?? ?? ?? ?? 8D 50 ?? 8B FF");
    address.musicVideoDirectory = ReadPtr32(ptr + 0x1);

    ptr = FindSignature(base, size, "C7 06 ?? ?? ?? ?? 88 9E ?? ?? ?? ?? 88 9E");
    address.stopMusicVideoDirectoryOverwrite = ptr + 0x36;

    ptr = FindSignature(base, size,
        "74 ?? 6A ?? DD D8 6A ?? 6A ?? 6A ?? 6A ?? 6A ?? 68 ?? ?? ?? ?? "
        "6A ?? 68 ?? ?? ?? ?? 68 ?? ?? ?? ?? E8 ?? ?? ?? ?? D9 44 24 ?? "
        "83 C4 ?? D9 9E ?? ?? ?? ?? 5E C2 ?? ?? CC CC CC CC CC CC CC CC");
    address.bypassFovOverflow = ptr;

    address.fov = FindSignature(base, size, "00 00 BE 42");
    address.climbFov = FindSignature(base, size, "00 00 DC 42");
    address.runSlideFov = FindSignature(base, size, "00 00 00 00 00 80 5B 40");

    address.patchConsole = FindSignature(base, size, "83 E0 ?? 09 81");
    address.patchPopupMenu0 = FindSignature(base, size, "74 ?? 8B 15 ?? ?? ?? ?? 8D 4C 24");
    address.patchPopupMenu1 = FindSignature(base, size, "09 05 ?? ?? ?? ?? 8B 95");
    address.patchRemoveFpsCap = FindSignature(base, size, "6A ?? FF 15 ?? ?? ?? ?? 8B 3D");
    address.patchInvertVerticalLook = FindSignature(base, size, "00 D9 45 00 74 06");

    ptr = FindSignature(base, size, "C6 82 ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? 83 F8");
    address.patchAutoSave = ptr - 0x6;

    address.patchAllowEmptySaveNames = FindSignature(base, size, "75 ?? 81 C6 ?? ?? ?? ?? 57");

    address.patchAllowBannedSaveNames = FindSignature(base, size, "74 ?? 8B 5C 24 ?? BE");

    address.patchAllowMultiInstances1 = FindSignature(base, size, "74 ?? 6a ?? 68 ?? ?? ?? ?? 68 ?? ?? ?? ?? 6a");
    address.PatchAllowMultiInstances2 = FindSignature(base, size, "75 ?? 8b 0d ?? ?? ?? ?? 68 ?? ?? ?? ?? 68 ?? ?? ?? ?? 68");

    address.patchFog = FindSignature(base, size, "0f 84 ?? ?? ?? ?? 80 78");

    address.patchForceMaxLod = FindSignature(base, size, "7e ?? f7 43");

    address.patchRemoveItemFreezing = FindSignature(base, size, "75 ?? d9 05 ?? ?? ?? ?? d8 9e");

    address.patchDefaultFarValue1 = FindSignature(base, size, "75 ?? d9 05 ?? ?? ?? ?? d9 5c 24 ?? 80 7c 24");
    address.patchDefaultFarValue2 = FindSignature(base, size, "74 ?? d9 44 24 ?? d9 9e ?? ?? ?? ?? d9 86");

    ptr = FindSignature(base, size, "20 41 ?? 00 c8");
    address.defaultFarValue = ptr + 0x2;

    address.patchRemoveCulling1 = FindSignature(base, size, "75 ?? 57 55 8b cb");
    address.patchRemoveCulling2 = FindSignature(base, size, "0f 84 ?? ?? ?? ?? 8b 85 ?? ?? ?? ?? 39 98");
    address.patchRemoveCulling3 = FindSignature(base, size, "7a ?? d8 97 ?? ?? ?? ?? df e0 f6 c4 ?? 7a ?? d9 e8");
    address.patchRemoveCulling4 = FindSignature(base, size, "7a ?? d9 e8 d8 5b ?? df e0 f6 c4 ?? 74");
    address.patchRemoveCulling5 = FindSignature(base, size, "7a ?? d8 5f");
    address.patchRemoveCulling6 = FindSignature(base, size, "7a ?? d9 e8 d8 59 ?? df e0 f6 c4 ?? 0f 85 ?? ?? ?? ?? eb ?? dd d8 53 56 8b 74 24 ?? 8b 9e ?? ?? ?? ?? 8b 43 ?? d9 40 ?? 83 c0 ?? d8 a6 ?? ?? ?? ?? 51 d9 5c 24 ?? d9 40 ?? d8 a6 ?? ?? ?? ?? d9 5c 24 ?? d9 40 ?? d8 a6 ?? ?? ?? ?? d9 5c 24 ?? d9 44 24 ?? d9 44 24 ?? d9 44 24 ?? d9 41 ?? d9 1c ?? d9 c1 de ca d9 c2 de cb d9 c9 de c2 dc c8 de c1 d9 5c 24 ?? d9 44 24 ?? e8 ?? ?? ?? ?? d9 5c 24 ?? d9 44 24 ?? 83 ec ?? d9 5c 24 ?? 8b cf d9 83 ?? ?? ?? ?? d8 8e ?? ?? ?? ?? d9 5c 24 ?? d9 44 24 ?? d9 5c 24 ?? d9 86 ?? ?? ?? ?? d9 5c 24 ?? d9 ee");

    address.patchNoBonks = FindSignature(base, size, "0f 85 ?? ?? ?? ?? 8b 86 ?? ?? ?? ?? 25 ?? ?? ?? ?? 33 c9 3d ?? ?? ?? ?? 75 ?? 3b cb 74");
}

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
        mov edx, dword ptr ds:[0x007df95c]//addresses.hClipCursor
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
        add dword ptr ds:[0x007df954]/*addresses.hFpsFix1*/, 0x1
        push 0x1
        call timeBeginPeriod
        jmp [jmpBackAddressFpsFix1]
    }
}

DWORD jmpBackAddressFpsFix2;
uint32_t cleanUp = 0x005c5170; //addresses.hFpsFix2CleanUp
void __declspec(naked) hFpsFix2() {
    __asm {
        call cleanUp
        push 0x1
        call timeEndPeriod
        jmp[jmpBackAddressFpsFix2]
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
    BYTE patchWindowed[] = {0xfe,0x85};
    BYTE patchWindowPosBorder[] = {0xfe,0xff,0xff,0xff};
    BYTE patchBorderless[] = {0x0a};
    BYTE patchWindowKey[] = {0x06};

    //Cursor only gets hidden when inside client area
    patch((BYTE*)addresses.patchCursorHide, addresses.patchCursorHidePatch.data(), addresses.patchCursorHidePatch.size());
    //Setting custom res
    if (cfg.displayMode != DisplayModes::Fullscreen) {
        patch((BYTE*)addresses.customResPatch1, zero, 1);
        patch((BYTE*)addresses.customResPatch2, reinterpret_cast<BYTE*>(&cfg.width), 2);
        patch((BYTE*)addresses.customResPatch3, reinterpret_cast<BYTE*>(&cfg.height), 2);
    }
    //Initial window position
    patch((BYTE*)addresses.initialWindowPositionPatch, zero, 1);
    //Not showing window before d3d9 device is created
    patch((BYTE*)addresses.notShowingWindowBeforeD3d9DeviceCreatedPatch1, zero, 1);
    patch((BYTE*)addresses.notShowingWindowBeforeD3d9DeviceCreatedPatch2, zero, 1);
    //Make game run windowed
    if (cfg.displayMode != DisplayModes::Fullscreen) {
        patch((BYTE*)addresses.patchWindowed, patchWindowed, 2);
    }
    //Set window position to top left corner
    patch((BYTE*)addresses.patchWindowPosBorderlessOrBorder, cfg.displayMode == DisplayModes::Borderless ? zero : patchWindowPosBorder, 4);
    if (cfg.displayMode == DisplayModes::Borderless) {
        patch((BYTE*)addresses.patchBorderless, patchBorderless, 1);
    }
    //Change directinput flag so windows key works
    patch((BYTE*)addresses.patchWindowKey, patchWindowKey, 1);

    //Bypass disc requirement
    patch((BYTE*)addresses.bypassDiscRequirementPatch1, nop, 6);
    patch((BYTE*)addresses.bypassDiscRequirementPatch2, nop, 2);
    patch((BYTE*)addresses.bypassDiscRequirementPatch3, jmp, 1);

    // Tell the game where the disc files can be found
    const size_t len = rootDirectory.length();
    BYTE* noCDDirectory = new BYTE[len + 1];

    for (size_t i = 0; i < len; ++i) {
        noCDDirectory[i] = static_cast<BYTE>(rootDirectory[i]);
    }
    noCDDirectory[len] = '\0';
    patch((BYTE*)addresses.musicVideoDirectory, noCDDirectory, len);

    delete[] noCDDirectory;

    // Stop directory from being overwritten
    // This one is scary because I don't exactly know what the purpose if this one is.
    // Please check this when you have the time.
    patch((BYTE*)addresses.stopMusicVideoDirectoryOverwrite, nop, 6);

    //Allow banned save names
    patch((BYTE*)addresses.patchAllowBannedSaveNames, jmp, 1);

    //Allow entering blank save name
    patch((BYTE*)addresses.patchAllowEmptySaveNames, jmp, 1);
    
    //Allow multiple game instances
    patch((BYTE*)addresses.patchAllowMultiInstances1, jmp, 1);
    patch((BYTE*)addresses.PatchAllowMultiInstances2, nop, 2);

    if (cfg.invertVerticalLook) {
        patch((BYTE*)addresses.patchInvertVerticalLook, (BYTE*)"\x75", 1);
    }

    if (!cfg.autoSave) {
        patch((BYTE*)addresses.patchAutoSave, nop, 13);
    }
}

void applyNonSpeedrunPatches(RatataRConfig& cfg) {
    // These patches will not get applied when speedrun mode is turned on!
    
    //Apply FOV
    patch((BYTE*)addresses.bypassFovOverflow, (BYTE*)"\xEB", 1); //BYPASS FOV OVERFLOW ERROR
    patch((BYTE*)addresses.fov, reinterpret_cast<BYTE*>(&cfg.fov), sizeof(float));
    patch((BYTE*)addresses.climbFov, reinterpret_cast<BYTE*>(&cfg.climbFOV), sizeof(float));
    patch((BYTE*)addresses.runSlideFov, reinterpret_cast<BYTE*>(&cfg.runSlideFOV), sizeof(double));

    if (cfg.improvedViewDistance) {
        //Use default far value
        patch((BYTE*)addresses.patchDefaultFarValue1, (BYTE*)"\x90\x90", 2);
        patch((BYTE*)addresses.patchDefaultFarValue2, (BYTE*)"\x90\x90", 2);

        //Set default far value to 500
        float defaultFar = 500.0f;
        patch((BYTE*)addresses.defaultFarValue, reinterpret_cast<BYTE*>(&defaultFar), sizeof(float));
        
        //Remove culling
        patch((BYTE*)addresses.patchRemoveCulling1, (BYTE*)"\xEB", 1);
        patch((BYTE*)addresses.patchRemoveCulling2, (BYTE*)"\x90\x90\x90\x90\x90\x90", 6);
        patch((BYTE*)addresses.patchRemoveCulling3, (BYTE*)"\x90\x90", 2);
        patch((BYTE*)addresses.patchRemoveCulling4, (BYTE*)"\x90\x90", 2);
        patch((BYTE*)addresses.patchRemoveCulling5, (BYTE*)"\x90\x90", 2);
        patch((BYTE*)addresses.patchRemoveCulling6, (BYTE*)"\x90\x90", 2);

        //Force max Lod
        patch((BYTE*)addresses.patchForceMaxLod, (BYTE*)"\xEB", 1);

        //Remove item freezing
        patch((BYTE*)addresses.patchRemoveItemFreezing, (BYTE*)"\xEB", 1);
    }

    if (!cfg.fog) {
        patch((BYTE*)addresses.patchFog, (BYTE*)"\xE9\x8F\x00\x00\x00\x90", 6);
    }
    
    if (cfg.noBonks) {
        patch((BYTE*)addresses.patchNoBonks, (BYTE*)"\xE9\x16\x01\x00\x00\x90", 6);
    }

    //Enable console
    if (cfg.console) {
        patch((BYTE*)addresses.patchConsole, (BYTE*)"\x90\x90\x90", 3);
    }

    if (cfg.popupMenu) {
        patch((BYTE*)addresses.patchPopupMenu0, (BYTE*)"\x90\x90", 2);
        patch((BYTE*)addresses.patchPopupMenu1, (BYTE*)"\x90\x90\x90\x90\x90\x90", 6);
    }

    if (cfg.removeFpsCap) {
        patch((BYTE*)addresses.patchRemoveFpsCap, (BYTE*)"\x90\x90\x90\x90\x90\x90\x90\x90", 8);
    }
}

DWORD WINAPI MainThread(LPVOID param) {
    TCHAR moduleFileName[MAX_PATH];
    GetModuleFileNameA((HMODULE)param, (LPSTR)moduleFileName, MAX_PATH);
    std::string::size_type pos = std::string((char*)moduleFileName).find_last_of("\\/");
    std::string configPath = std::string((char*)moduleFileName).substr(0, pos).append("\\").append("RatataRconfig.ini");
    rootDirectory = std::string((char*)moduleFileName).substr(0, pos);

    getSignatures(addresses);

    DWORD hookAddressCursor = addresses.hookAddressCursor;
    DWORD hookAddressSetWindowPosPush = addresses.hookAddressSetWindowPosPush;
    DWORD hookAddressConsoleEnable = addresses.hookAddressConsoleEnable;
    DWORD hookAddressShowConsole = addresses.hookAddressShowConsole;
    DWORD hookAddressFpsFix1 = addresses.hookAddressFpsFix1;
    DWORD hookAddressFpsFix2 = addresses.hookAddressFpsFix2;
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

    // The commented hooks have not been implemented yet.

    //hook((void*)hookAddressCursor,hClipCursor,hookLengthCursor);
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

    //hook((void*)hookAddressFpsFix1,hFpsFix1,hookLengthFpsFix1);
    //hook((void*)hookAddressFpsFix2,hFpsFix2,hookLengthFpsFix2);

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
