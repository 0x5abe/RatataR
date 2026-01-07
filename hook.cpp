#include <Windows.h>
#include <iostream>
#include <timeapi.h>
#include <Psapi.h>
#include <vector>
#include <sstream>
#include <d3d9.h>
#include "rich-presence/rpc.h"
#include "DynArray_Z.h"
#include "MinHook.h"

HANDLE readyEvent = nullptr;

using EndScene_t = HRESULT(__stdcall*)(IDirect3DDevice9*);
EndScene_t static oEndScene = nullptr;

// Frame limiter
static LARGE_INTEGER g_QPCFreq;
static LARGE_INTEGER g_NextFrameTime;
static bool g_TimerInitialized = false;
static bool g_EnableFrameLimit = false;
static double g_TargetFPS = 60.0;

bool SigFail(const char* name, const char* pattern) {
    char buf[1024];
    sprintf_s(
        buf,
        "Signature NOT FOUND:\n\n%s\n\nPattern:\n%s",
        name,
        pattern
    );
    MessageBoxA(nullptr, buf, "Signature error", MB_ICONERROR | MB_OK);
    return false;
}

#define FIND_SIG_OR_FAIL(var, pattern)            \
        var = FindSignature(base, size, pattern); \
        if (!(var)) return SigFail(#var, pattern) \

#define FIND_SIG(var, pattern)                   \
        var = FindSignature(base, size, pattern) \

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

    uintptr_t levelIdBaseAddr;
    uintptr_t playerObjectsAddr;
    uintptr_t getIDAddr;

    uintptr_t systemDatasPtrAddr;
    uintptr_t rdrPtrAddr;
    uintptr_t classMgrPtrAddr;
    uintptr_t getPtrAddr;
    uintptr_t drawStringAddr;

    uintptr_t patchDrawFpsAddr;
};

std::string rootDirectory;
DWORD levelIdBaseAddr;
DWORD playerObjectsAddr;
DWORD getIDAddr;
PatchAddresses addresses{};

enum DisplayModes {
    Windowed,
    Borderless,
    Fullscreen
};

struct RemyFOV {
    float normal;
    float climbing;
    double runningSliding;
};
RemyFOV g_Fov = { 95.0f, 110.0f, 110.0};

struct RatataRConfig {
    unsigned int width;
    unsigned int height;
    unsigned int maxFps;
    unsigned int fov;
    bool console;
    bool popupMenu;
    bool invertVerticalLook;
    bool removeFpsCap;
    bool autoSave;
    bool improvedViewDistance;
    bool fog;
    bool noBonks;
    bool speedrunMode;
    bool discordRichPresence;
    bool displayFrameCounter;
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

void loadConfig(RatataRConfig& cfg, const std::string& configPath) {
    const char* sectionName = "CONFIG";

    IntOption intOptions[] = {
        {"width",                       &RatataRConfig::width,                            0},
        {"height",                      &RatataRConfig::height,                           0},
        {"maxFps",                      &RatataRConfig::maxFps,                           0},
        {"fov",                         &RatataRConfig::fov,                              95},
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
        {"discordRichPresence",         &RatataRConfig::discordRichPresence,           false},
        {"displayFrameCounter",         &RatataRConfig::displayFrameCounter,           false},
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

    // Field Of View
    constexpr float fovMin = 1.0f;
    constexpr float fovMax = 155.0f;
    constexpr float CLIMBING_RATIO = 110.0f / 95.0f;
    constexpr double RUNNING_SLIDING_RATIO = 110.0 / 95.0;
    float userFOV = static_cast<float>(cfg.fov);

    // Clamp
    if (userFOV < fovMin) userFOV = fovMin;
    else if (userFOV > fovMax) userFOV = fovMax;

    g_Fov.normal = userFOV;
    g_Fov.climbing = CLIMBING_RATIO * userFOV;
    g_Fov.runningSliding = RUNNING_SLIDING_RATIO * userFOV;

    // Set to screen resolution if width is 0
    if (cfg.width == 0) {
        cfg.width = GetSystemMetrics(SM_CXSCREEN);
    }

    // Set to screen resolution if height is 0
    if (cfg.height == 0) {
        cfg.height = GetSystemMetrics(SM_CYSCREEN);
    }

    // Frame Limit
    g_EnableFrameLimit = cfg.maxFps > 0.0;
    g_TargetFPS = static_cast<double>(cfg.maxFps);
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

bool getSignatures(PatchAddresses& address)
{
    auto mainMod = GetMainModule();
    const uintptr_t base = mainMod.base;
    const size_t size = mainMod.size;

    uintptr_t ptr = 0;

    FIND_SIG_OR_FAIL(ptr, "8B 15 ?? ?? ?? ?? 8D 4C 24");
    address.hookAddressCursor = ptr + 0x0C;
    address.hClipCursor = ReadPtr32(ptr + 0x2);

    FIND_SIG_OR_FAIL(address.hookAddressSetWindowPosPush, "6A ?? 03 C7 50 55");
    FIND_SIG_OR_FAIL(address.hookAddressConsoleEnable,   "09 81 ?? ?? ?? ?? C2 ?? ?? 81 EC");
    FIND_SIG_OR_FAIL(address.hookAddressShowConsole,     "21 9E ?? ?? ?? ?? 5E");

    FIND_SIG_OR_FAIL(ptr, "83 05 ?? ?? ?? ?? ?? 80 3D");
    address.hookAddressFpsFix1 = ptr;
    address.hookAddressFpsFix2 = ptr + 0x1E;
    address.hFpsFix1 = ReadPtr32(ptr + 0x2);

    FIND_SIG_OR_FAIL(address.hFpsFix2CleanUp, "53 56 57 E8 ?? ?? ?? ?? 8B 0D");

    ptr = FindSignature(base, size, "F6 05 ?? ?? ?? ?? ?? 0F 84 ?? ?? ?? ?? 6A");
    if (ptr) {
        address.patchCursorHide = ptr;
        address.patchCursorHidePatch = { 0x66,0x81,0x7C,0x24,0x18,0x01,0x00,0x0F,0x85 };
    } else {
        FIND_SIG_OR_FAIL(ptr, "F6 05 ?? ?? ?? ?? ?? 74 ?? 6A");
        address.patchCursorHide = ptr;
        address.patchCursorHidePatch = { 0x66,0x81,0x7C,0x24,0x18,0x01,0x00,0x75 };
    }

    FIND_SIG_OR_FAIL(ptr, "0E 66 C7 44 24");
    address.customResPatch1 = ptr;
    address.customResPatch2 = ptr + 0x6;
    address.customResPatch3 = ptr + 0x0D;

    FIND_SIG_OR_FAIL(address.initialWindowPositionPatch, "80 68 ?? ?? C2");

    FIND_SIG_OR_FAIL(ptr, "80 68 ?? ?? 00 80");
    address.notShowingWindowBeforeD3d9DeviceCreatedPatch1 = ptr + 0xA;
    address.notShowingWindowBeforeD3d9DeviceCreatedPatch2 = ptr;

    FIND_SIG_OR_FAIL(address.patchWindowed, "89 8D ?? ?? ?? ?? 89 0E");
    FIND_SIG_OR_FAIL(address.patchWindowPosBorderlessOrBorder, "AE 01 00 00 6A");
    FIND_SIG_OR_FAIL(address.patchBorderless, "CA ?? ?? F0 ?? FF 15");
    FIND_SIG_OR_FAIL(address.patchWindowKey, "16 51 50");

    FIND_SIG_OR_FAIL(address.bypassDiscRequirementPatch1, "0F 85 ?? ?? ?? ?? 8D 84 24 ?? ?? ?? ?? 8B D6");

    FIND_SIG_OR_FAIL(ptr, "74 ?? 53 53 8D 44 24");
    address.bypassDiscRequirementPatch2 = ptr;
    address.bypassDiscRequirementPatch3 = ptr + 0x3E;

    FIND_SIG_OR_FAIL(ptr, "B8 ?? ?? ?? ?? 8D 50 ?? 8B FF");
    address.musicVideoDirectory = ReadPtr32(ptr + 0x1);

    FIND_SIG_OR_FAIL(ptr, "C7 06 ?? ?? ?? ?? 88 9E ?? ?? ?? ?? 88 9E");
    address.stopMusicVideoDirectoryOverwrite = ptr + 0x36;

    FIND_SIG_OR_FAIL(address.bypassFovOverflow,
        "74 ?? 6A ?? DD D8 6A ?? 6A ?? 6A ?? 6A ?? 6A ?? 68 ?? ?? ?? ?? "
        "6A ?? 68 ?? ?? ?? ?? 68 ?? ?? ?? ?? E8 ?? ?? ?? ?? D9 44 24 ?? "
        "83 C4 ?? D9 9E ?? ?? ?? ?? 5E C2 ?? ?? CC CC CC CC CC CC CC CC");

    FIND_SIG_OR_FAIL(address.fov,        "00 00 BE 42");
    FIND_SIG_OR_FAIL(address.climbFov,   "00 00 DC 42");
    FIND_SIG_OR_FAIL(address.runSlideFov,"00 00 00 00 00 80 5B 40");

    FIND_SIG_OR_FAIL(address.patchConsole, "83 E0 ?? 09 81");
    FIND_SIG_OR_FAIL(address.patchPopupMenu0, "74 ?? 8B 15 ?? ?? ?? ?? 8D 4C 24");
    FIND_SIG_OR_FAIL(address.patchPopupMenu1, "09 05 ?? ?? ?? ?? 8B 95");
    FIND_SIG_OR_FAIL(address.patchRemoveFpsCap, "6A ?? FF 15 ?? ?? ?? ?? 8B 3D");
    FIND_SIG_OR_FAIL(address.patchInvertVerticalLook, "00 D9 45 00 74 06");

    FIND_SIG_OR_FAIL(ptr, "C6 82 ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? 83 F8");
    address.patchAutoSave = ptr - 0x6;

    FIND_SIG_OR_FAIL(address.patchAllowEmptySaveNames, "75 ?? 81 C6 ?? ?? ?? ?? 57");
    FIND_SIG_OR_FAIL(address.patchAllowBannedSaveNames, "74 ?? 8B 5C 24 ?? BE");

    // Currently optional
    FIND_SIG(address.patchAllowMultiInstances1,
        "74 ?? 6a ?? 68 ?? ?? ?? ?? 68 ?? ?? ?? ?? 6a");
    FIND_SIG(address.PatchAllowMultiInstances2,
        "75 ?? 8b 0d ?? ?? ?? ?? 68 ?? ?? ?? ?? 68 ?? ?? ?? ?? 68");

    FIND_SIG_OR_FAIL(address.patchFog, "0f 84 ?? ?? ?? ?? 80 78");
    FIND_SIG_OR_FAIL(address.patchForceMaxLod, "7e ?? f7 43");
    FIND_SIG_OR_FAIL(address.patchRemoveItemFreezing, "75 ?? d9 05 ?? ?? ?? ?? d8 9e");

    FIND_SIG_OR_FAIL(address.patchDefaultFarValue1,
        "75 ?? d9 05 ?? ?? ?? ?? d9 5c 24 ?? 80 7c 24");
    FIND_SIG_OR_FAIL(address.patchDefaultFarValue2,
        "74 ?? d9 44 24 ?? d9 9e ?? ?? ?? ?? d9 86");

    FIND_SIG_OR_FAIL(ptr, "20 41 ?? 00 c8");
    address.defaultFarValue = ptr + 0x2;

    FIND_SIG_OR_FAIL(address.patchRemoveCulling1, "75 ?? 57 55 8b cb");
    FIND_SIG_OR_FAIL(address.patchRemoveCulling2, "0f 84 ?? ?? ?? ?? 8b 85 ?? ?? ?? ?? 39 98");
    FIND_SIG_OR_FAIL(address.patchRemoveCulling3, "7a ?? d8 97 ?? ?? ?? ?? df e0 f6 c4 ?? 7a ?? d9 e8");
    FIND_SIG_OR_FAIL(address.patchRemoveCulling4, "7a ?? d9 e8 d8 5b ?? df e0 f6 c4 ?? 74");
    FIND_SIG_OR_FAIL(address.patchRemoveCulling5, "7a ?? d8 5f");
    FIND_SIG_OR_FAIL(address.patchRemoveCulling6,
        "7a ?? d9 e8 d8 59 ?? df e0 f6 c4 ?? 0f 85 ?? ?? ?? ?? eb ?? dd d8 53 56 8b 74 24 ?? 8b 9e ?? ?? ?? ?? 8b 43 ?? d9 40 ?? 83 c0 ?? d8 a6 ?? ?? ?? ?? 51 d9 5c 24 ?? d9 40 ?? d8 a6 ?? ?? ?? ?? d9 5c 24 ?? d9 40 ?? d8 a6 ?? ?? ?? ?? d9 5c 24 ?? d9 44 24 ?? d9 44 24 ?? d9 44 24 ?? d9 41 ?? d9 1c ?? d9 c1 de ca d9 c2 de cb d9 c9 de c2 dc c8 de c1 d9 5c 24 ?? d9 44 24 ?? e8 ?? ?? ?? ?? d9 5c 24 ?? d9 44 24 ?? 83 ec ?? d9 5c 24 ?? 8b cf d9 83 ?? ?? ?? ?? d8 8e ?? ?? ?? ?? d9 5c 24 ?? d9 44 24 ?? d9 5c 24 ?? d9 86 ?? ?? ?? ?? d9 5c 24 ?? d9 ee");

    FIND_SIG_OR_FAIL(address.patchNoBonks,
        "0f 85 ?? ?? ?? ?? 8b 86 ?? ?? ?? ?? 25 ?? ?? ?? ?? 33 c9 3d ?? ?? ?? ?? 75 ?? 3b cb 74");

    FIND_SIG_OR_FAIL(ptr, "8B 0D ?? ?? ?? ?? 50 E8 ?? ?? ?? ?? 8B 48 ?? 89 4E");
    address.levelIdBaseAddr = ReadPtr32(ptr + 0x2);

    FIND_SIG_OR_FAIL(address.getIDAddr, "8b 54 24 ?? 8a 0a");

    FIND_SIG_OR_FAIL(ptr, "8b 15 ?? ?? ?? ?? 8b 1c 82");
    address.playerObjectsAddr = ReadPtr32(ptr + 0x2);

    FIND_SIG_OR_FAIL(address.systemDatasPtrAddr, "8b 0d ?? ?? ?? ?? 85 c9 74 ?? e8 ?? ?? ?? ?? 8b 0d ?? ?? ?? ?? 85 c9 74 ?? e8 ?? ?? ?? ?? 8b 0d ?? ?? ?? ?? 85 c9 74 ?? 8b 11 8b 42 ?? ff d0 8b 0d");
    FIND_SIG_OR_FAIL(address.rdrPtrAddr, "a1 ?? ?? ?? ?? 8b 90 ?? ?? ?? ?? 8b 80 ?? ?? ?? ?? 89 54 24");
    FIND_SIG_OR_FAIL(address.classMgrPtrAddr, "8b 0d ?? ?? ?? ?? e8 ?? ?? ?? ?? c2 ?? ?? cc cc cc cc cc cc 8b 44 24");
    FIND_SIG_OR_FAIL(address.getPtrAddr, "83 05 ?? ?? ?? ?? ?? 8b 54 24");
    FIND_SIG_OR_FAIL(address.drawStringAddr, "55 8b ec 83 e4 ?? 81 ec ?? ?? ?? ?? 53 56 8b 75 ?? 80 7e ?? ?? d9 46");

    FIND_SIG_OR_FAIL(address.patchDrawFpsAddr, "e8 ?? ?? ?? ?? 8b e5 5d c2 ?? ?? cc cc cc 83 ec");

    return true;
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

DWORD hClipCursorAddress;
DWORD jmpBackAddressCursor;
void __declspec(naked) hClipCursor() {
    __asm {
        call GetClientRect
        lea ecx, [esp+0x1c]
        mov eax, dword ptr[hClipCursorAddress]
        mov edx, dword ptr[eax]
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

DWORD hFpsFix1Addr;
DWORD jmpBackAddressFpsFix1;
void __declspec(naked) hFpsFix1() {
    __asm {
        mov eax, [hFpsFix1Addr]
        add dword ptr[eax], 0x1
        push 0x1
        call timeBeginPeriod
        jmp[jmpBackAddressFpsFix1]
    }
}

DWORD jmpBackAddressFpsFix2;
DWORD cleanUp;
void __declspec(naked) hFpsFix2() {
    __asm {
        mov eax, [cleanUp]
        call eax
        push 0x1
        call timeEndPeriod
        jmp[jmpBackAddressFpsFix2]
    }
}

void InitTimers() {
    QueryPerformanceFrequency(&g_QPCFreq);
    QueryPerformanceCounter(&g_NextFrameTime);
    g_TimerInitialized = true;
}

void ApplyFrameLimit() {
    const double targetFrameTime = 1.0 / g_TargetFPS;
    const LONGLONG targetTicks = static_cast<LONGLONG>(targetFrameTime * g_QPCFreq.QuadPart);

    LARGE_INTEGER now;

    while (true) {
        QueryPerformanceCounter(&now);

        LONGLONG remainingTicks = g_NextFrameTime.QuadPart - now.QuadPart;
        if (remainingTicks <= 0)
            break;

        double remainingSeconds = double(remainingTicks) / double(g_QPCFreq.QuadPart);
        
        if (remainingSeconds > 0.001) {
            Sleep(DWORD((remainingSeconds - 0.001) * 1000.0));
        }
        else {
            YieldProcessor();
        }
    }

    g_NextFrameTime.QuadPart += targetTicks;

    QueryPerformanceCounter(&now);
    if (g_NextFrameTime.QuadPart < now.QuadPart - targetTicks) {
        g_NextFrameTime = now;
    }
}

HRESULT __stdcall hkEndScene(IDirect3DDevice9* pDevice) {
    if (!g_TimerInitialized)
        InitTimers();

    if (g_EnableFrameLimit)
        ApplyFrameLimit();

    return oEndScene(pDevice);
}

bool HookD3D9EndScene() {
    IDirect3D9* pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!pD3D) return false;

    WNDCLASSEXA wc = {
        sizeof(WNDCLASSEXA),
        CS_CLASSDC,
        DefWindowProcA,
        0L, 0L,
        GetModuleHandle(nullptr),
        nullptr, nullptr, nullptr, nullptr,
        "DummyWindow",
        nullptr
    };
    RegisterClassExA(&wc);

    HWND hWnd = CreateWindowA(
        "DummyWindow",
        nullptr,
        WS_OVERLAPPEDWINDOW,
        0, 0, 100, 100,
        nullptr, nullptr, wc.hInstance, nullptr
    );

    D3DPRESENT_PARAMETERS d3dpp{};
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.hDeviceWindow = hWnd;

    IDirect3DDevice9* pDevice = nullptr;
    if (FAILED(pD3D->CreateDevice(
        D3DADAPTER_DEFAULT,
        D3DDEVTYPE_HAL,
        hWnd,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING,
        &d3dpp,
        &pDevice)))
    {
        DestroyWindow(hWnd);
        UnregisterClassA("DummyWindow", wc.hInstance);
        pD3D->Release();
        return false;
    }

    void** vTable = *reinterpret_cast<void***>(pDevice);
    void* pEndScene = vTable[42];

    MH_Initialize();
    MH_CreateHook(pEndScene, &hkEndScene, reinterpret_cast<void**>(&oEndScene));
    MH_EnableHook(pEndScene);

    pDevice->Release();
    pD3D->Release();
    DestroyWindow(hWnd);
    UnregisterClassA("DummyWindow", wc.hInstance);

    return true;
}

void patch(BYTE* ptr, BYTE* buf, size_t len) {
    DWORD curProtection;
    VirtualProtect(ptr, len, PAGE_EXECUTE_READWRITE, &curProtection);
    memcpy(ptr, buf, len);
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
    if (addresses.patchAllowMultiInstances1 && addresses.PatchAllowMultiInstances2) {
        patch((BYTE*)addresses.patchAllowMultiInstances1, jmp, 1);
        patch((BYTE*)addresses.PatchAllowMultiInstances2, nop, 2);
    }

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
    patch((BYTE*)addresses.fov, reinterpret_cast<BYTE*>(&g_Fov.normal), sizeof(g_Fov.normal));
    patch((BYTE*)addresses.climbFov, reinterpret_cast<BYTE*>(&g_Fov.climbing), sizeof(g_Fov.climbing));
    patch((BYTE*)addresses.runSlideFov, reinterpret_cast<BYTE*>(&g_Fov.runningSliding), sizeof(g_Fov.runningSliding));

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

static bool PatchCall32(void* callInstrAddr, void* newTarget)
{
    auto p = reinterpret_cast<std::uint8_t*>(callInstrAddr);

    if (p[0] != 0xE8) return false;

    DWORD oldProt = 0;
    if (!VirtualProtect(p, 5, PAGE_EXECUTE_READWRITE, &oldProt))
        return false;

    std::int32_t rel = (std::int32_t)(
        (std::uintptr_t)newTarget - ((std::uintptr_t)callInstrAddr + 5)
    );

    p[0] = 0xE8;
    std::memcpy(p + 1, &rel, sizeof(rel));

    FlushInstructionCache(GetCurrentProcess(), p, 5);

    DWORD tmp = 0;
    VirtualProtect(p, 5, oldProt, &tmp);
    return true;
}

typedef unsigned int hdl;

struct Color {
    float r, g, b, a;
};

struct Vec2f {
    float x, y;
};

struct FontParam_Z {
    char* text;
    bool hasBorder;
    float borderOffset;
    Color borderColor;
    float topBoundY;
    float bottomBoundY;
    Vec2f position;
    Color bottomColor;
    Color topColor;
    float scale1;
    float scale2;
    float zOffset;
    Vec2f finalPosition;
};

char** systemDatasPtr = (char**)0x7de8dc;
char** rdrPtr = (char**)0x7de8a4;
char** classMgrPtr = (char**)0x7de8a8;
char* (__fastcall* getPtr)(char* classMgr, int unused, hdl* id) = (char* (__fastcall*)(char*, int, hdl*))0x653280;
char* (__fastcall* drawString)(char* font, int unused, FontParam_Z* fontParam) = (char* (__fastcall*)(char*, int, FontParam_Z*))0x5ce060;

void __fastcall DrawFps() {
    if (*systemDatasPtr == NULL || *rdrPtr == NULL || *classMgrPtr == NULL || getPtr == 0 || drawString == 0) {
        return;
    }
    DynArray_Z<hdl>* fonts = (DynArray_Z<hdl>*)(*systemDatasPtr + 0x18);
    if (fonts->GetSize() == 0) {
        return;
    }
    char* font = getPtr(*classMgrPtr, 0, &fonts->Get(0));
    if (font == NULL) {
        return;
    }
    Color purple = { 0.4f, 0.1f, 1.0f, 1.0f };
    Color otherPurple = { 0.1f, 0.1f, 1.0f, 1.0f };
    Color black = { 0.0f, 0.0f, 0.0f, 1.0f };
    Vec2f position = { 4.0f, -2.0f };
    FontParam_Z fontParam;
    char fpsString[32];
    float fps = *(float*)(*rdrPtr+0xc34);

    _snprintf_s(
        fpsString,
        sizeof(fpsString),
        _TRUNCATE,
        "%.0f",
        fps
    );
    fontParam.text = fpsString;
    // possibly have an option to toggle border
    fontParam.hasBorder = true;
    fontParam.borderOffset = 2.0f;
    // possibly have an option to change border color
    fontParam.borderColor = black;
    fontParam.topBoundY = -1.0f;
    fontParam.bottomBoundY = -1.0f;
    fontParam.position = position;
    // possibly have an option to change colors
    fontParam.bottomColor = purple;
    fontParam.topColor = otherPurple;
    // possibly have an option to change scale
    fontParam.scale1 = 0.8f;
    fontParam.scale2 = 1.0f;
    fontParam.zOffset = 0.f;
    drawString(font, 0, &fontParam);
}

void ApplyHooks(RatataRConfig& cfg) {
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
    jmpBackAddressCursor = hookAddressCursor + hookLengthCursor;
    jmpBackAddressSetWindowPosPush = hookAddressSetWindowPosPush + hookLengthSetWindowPosPush;
    jmpBackAddressConsoleEnable = hookAddressConsoleEnable + hookLengthConsoleEnable;
    jmpBackAddressShowConsole = hookAddressShowConsole + hookLengthShowConsole;
    jmpBackAddressFpsFix1 = hookAddressFpsFix1 + hookLengthFpsFix1;
    jmpBackAddressFpsFix2 = hookAddressFpsFix2 + hookLengthFpsFix2;
    hClipCursorAddress = addresses.hClipCursor;
    hFpsFix1Addr = addresses.hFpsFix1;
    cleanUp = addresses.hFpsFix2CleanUp;
    levelIdBaseAddr = addresses.levelIdBaseAddr;
    playerObjectsAddr = addresses.playerObjectsAddr;
    getIDAddr = addresses.getIDAddr;

    hook((void*)hookAddressCursor, hClipCursor, hookLengthCursor);
    if (cfg.displayMode == DisplayModes::Borderless) {
        hook((void*)hookAddressSetWindowPosPush, hSetWindowPosPushBL, hookLengthSetWindowPosPush);
    }

    else if (cfg.displayMode != DisplayModes::Fullscreen) {
        hook((void*)hookAddressSetWindowPosPush, hSetWindowPosPushWND, hookLengthSetWindowPosPush);
    }

    if (cfg.console) {
        hook((void*)hookAddressConsoleEnable, hEnableConsole, hookLengthConsoleEnable);
        hook((void*)hookAddressShowConsole, hShowConsole, hookLengthShowConsole);
    }

    hook((void*)hookAddressFpsFix1, hFpsFix1, hookLengthFpsFix1);
    hook((void*)hookAddressFpsFix2, hFpsFix2, hookLengthFpsFix2);

    // Start rich presence thread
    if (cfg.discordRichPresence) {
        CreateThread(nullptr, 0, InitRPC, nullptr, 0, nullptr);
    }

    if (cfg.displayFrameCounter) {
        PatchCall32((void*)addresses.patchDrawFpsAddr, (void*)&DrawFps);
    }

    HookD3D9EndScene();
}

DWORD WINAPI MainThread(LPVOID param) {
    TCHAR moduleFileName[MAX_PATH];
    GetModuleFileNameA((HMODULE)param, (LPSTR)moduleFileName, MAX_PATH);
    std::string::size_type pos = std::string((char*)moduleFileName).find_last_of("\\/");
    std::string configPath = std::string((char*)moduleFileName).substr(0, pos).append("\\").append("RatataRconfig.ini");
    rootDirectory = std::string((char*)moduleFileName).substr(0, pos);

    if (!getSignatures(addresses)) {
        SetEvent(readyEvent);
        return 0;
    }
    
    RatataRConfig config;
    loadConfig(config, configPath);

    applyBasePatches(config);
    if (!config.speedrunMode) {
        applyNonSpeedrunPatches(config);
    }
    ApplyHooks(config);

    SetEvent(readyEvent);

    return 0;
}

BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpReserved) {
    switch (dwReason) {
        case DLL_PROCESS_ATTACH:
            readyEvent = CreateEventA(nullptr, TRUE, FALSE, "RatataR_Patched");
            DisableThreadLibraryCalls(hModule);
            CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr);
            break;
    }
    return TRUE;
}
