#include <Windows.h>
#include <iostream>
#include <timeapi.h>
#include <Psapi.h>
#include <vector>
#include <sstream>
#include <d3d9.h>
#include <algorithm>
#include "rpc-client/rpc.hpp"
#include "rpc-client/levels.hpp"
#include "Zouna/DynArray_Z.hpp"
#include "MinHook.h"
#include "ConfigHandler.hpp"
#include "MemoryUtils.hpp"
#include "SigScanner.hpp"
#include "SignaturePatterns.hpp"

typedef unsigned int hdl;

static char** systemDatasPtr = nullptr;
static char** rdrPtr = nullptr;
static char** classMgrPtr = nullptr;
static char* (__fastcall* getPtr)(char* classMgr, int unused, hdl* id) = nullptr;
static char* (__fastcall* drawStringFn)(char* font, int unused, FontParam_Z* fontParam) = nullptr;

using EndScene_t = HRESULT(__stdcall*)(IDirect3DDevice9*);
EndScene_t static oEndScene = nullptr;

// Frame limiter
static LARGE_INTEGER g_QPCFreq;
static LARGE_INTEGER g_NextFrameTime;
static bool g_TimerInitialized = false;
static bool g_EnableFrameLimit = false;
static double g_TargetFPS = 60.0;

std::vector<BYTE> CursorHidePatchBytes;

struct HookEntry {
    uintptr_t address;
    size_t length;
    void* detour;
    uintptr_t* trampoline;
    bool condition = true;
};

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

    uintptr_t hookAddressDeferredCasterDraw;

    uintptr_t patchCursorHide;

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
    uintptr_t patchAllowMultiInstances2;

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

    uintptr_t levelIdBase;
    uintptr_t playerObjects;
    uintptr_t getID;

    uintptr_t systemDatasPtr;
    uintptr_t rdrPtr;
    uintptr_t classMgrPtr;
    uintptr_t getFontPtr;
    uintptr_t drawStringFn;

    uintptr_t patchDrawFps;
};

std::string rootDirectory;
PatchAddresses addresses{};

struct ModuleInfo {
    uintptr_t base;
    size_t size;
};

ModuleInfo GetMainModule()
{
    HMODULE mainMod = GetModuleHandle(nullptr);

    MODULEINFO info{};
    GetModuleInformation(GetCurrentProcess(), mainMod, &info, sizeof(info));

    return {
        .base = (uintptr_t)info.lpBaseOfDll,
        .size = (size_t)info.SizeOfImage
    };
}

void getSignatures(PatchAddresses& address)
{
    auto mainMod = GetMainModule();
    const uintptr_t base = mainMod.base;
    const size_t size = mainMod.size;

    uintptr_t ptr = 0;

    ptr = FindSignature(base, size, hookAddressCursorSig);
    if (ptr) {
        address.hookAddressCursor = ptr + 0x0C;
        address.hClipCursor = ReadPtr32(ptr + 0x2);
    }

    ptr = FindSignature(base, size, hookAddressSetWindowPosPushSig);
    if (ptr)
        address.hookAddressSetWindowPosPush = ptr;

    ptr = FindSignature(base, size, hookAddressConsoleEnableSig);
    if (ptr)
        address.hookAddressConsoleEnable = ptr;

    ptr = FindSignature(base, size, hookAddressShowConsoleSig);
    if (ptr)
        address.hookAddressShowConsole = ptr;

    ptr = FindSignature(base, size, hookAddressFpsFixSig);
    if (ptr) {
        address.hookAddressFpsFix1 = ptr;
        address.hookAddressFpsFix2 = ptr + 0x1E;
        address.hFpsFix1 = ReadPtr32(ptr + 0x2);
    }

    ptr = FindSignature(base, size, hFpsFix2CleanUpSig);
    if (ptr)
        address.hFpsFix2CleanUp = ptr;

    // TODO: Replace the hardcoded address with this signature scan.
    // ptr = FindSignature(base, size, hookAddressDeferredCasterDrawSig);
    address.hookAddressDeferredCasterDraw = 0x006792E7;

    ptr = FindSignature(base, size, patchCursorHideSig1);
    if (ptr) {
        address.patchCursorHide = ptr;
        CursorHidePatchBytes = { 0x66,0x81,0x7C,0x24,0x18,0x01,0x00,0x0F,0x85 };
    } else {
        ptr = FindSignature(base, size, patchCursorHideSig2);
        address.patchCursorHide = ptr;
        CursorHidePatchBytes = { 0x66,0x81,0x7C,0x24,0x18,0x01,0x00,0x75 };
    }

    ptr = FindSignature(base, size, customResPatchSig);
    if (ptr) {
        address.customResPatch1 = ptr;
        address.customResPatch2 = ptr + 0x6;
        address.customResPatch3 = ptr + 0x0D;
    }

    ptr = FindSignature(base, size, initialWindowPosPatchSig);
    if (ptr)
        address.initialWindowPositionPatch = ptr;

    ptr = FindSignature(base, size, notShowingWindowBeforeD3d9DeviceCreatedPatchSig);
    if (ptr) {
        address.notShowingWindowBeforeD3d9DeviceCreatedPatch1 = ptr + 0xA;
        address.notShowingWindowBeforeD3d9DeviceCreatedPatch2 = ptr;
    }

    ptr = FindSignature(base, size, patchWindowedSig);
    if (ptr)
        address.patchWindowed = ptr;

    ptr = FindSignature(base, size, patchWindowPosBorderlessOrBorderSig);
    if (ptr)
        address.patchWindowPosBorderlessOrBorder = ptr;

    ptr = FindSignature(base, size, patchBorderlessSig);
    if (ptr)
        address.patchBorderless = ptr;

    ptr = FindSignature(base, size, patchWindowKeySig);
    if (ptr)
        address.patchWindowKey = ptr;

    ptr = FindSignature(base, size, bypassDiscRequirementPatchSig1);
    address.bypassDiscRequirementPatch1 = ptr;

    ptr = FindSignature(base, size, bypassDiscRequirementPatchSig2);
    if (ptr) {
        address.bypassDiscRequirementPatch2 = ptr;
        address.bypassDiscRequirementPatch3 = ptr + 0x3E;
    }

    ptr = FindSignature(base, size, discFileDirectorySig);
    if (ptr)
        address.musicVideoDirectory = ReadPtr32(ptr + 0x1);

    ptr = FindSignature(base, size, discFileDirectoryOverwriteSig);
    if (ptr) {
        address.stopMusicVideoDirectoryOverwrite = ptr + 0x36;
    }

    ptr = FindSignature(base, size, bypassFovOverflowSig);
    if (ptr)
        address.bypassFovOverflow = ptr;

    ptr = FindSignature(base, size, fovSig);
    if (ptr)
        address.fov = ptr;

    ptr = FindSignature(base, size, climbFovSig);
    if (ptr)
        address.climbFov = ptr;

    ptr = FindSignature(base, size, runSlideFovSig);
    if (ptr)
        address.runSlideFov = ptr;

    ptr = FindSignature(base, size, patchConsoleSig);
    if (ptr)
        address.patchConsole = ptr;

    ptr = FindSignature(base, size, patchPopupMenuSig1);
    if (ptr)
        address.patchPopupMenu0 = ptr;

    ptr = FindSignature(base, size, patchPopupMenuSig2);
    if (ptr)
        address.patchPopupMenu1 = ptr;

    ptr = FindSignature(base, size, patchRemoveFpsCapSig);
    if (ptr)
        address.patchRemoveFpsCap = ptr;

    ptr = FindSignature(base, size, patchInvertVerticalLookSig);
    if (ptr)
        address.patchInvertVerticalLook = ptr;

    ptr = FindSignature(base, size, patchAutoSaveSig);
    if (ptr)
        address.patchAutoSave = ptr - 0x6;

    ptr = FindSignature(base, size, patchAllowEmptySaveNamesSig);
    if (ptr)
        address.patchAllowEmptySaveNames = ptr;
    
    ptr = FindSignature(base, size, patchAllowBannedSaveNamesSig);
    if (ptr)
        address.patchAllowBannedSaveNames = ptr;

    ptr = FindSignature(base, size, patchAllowMultiInstancesSig1);
    if (ptr)
        address.patchAllowMultiInstances1 = ptr;

    ptr = FindSignature(base, size, patchAllowMultiInstancesSig2);
    if (ptr)
        address.patchAllowMultiInstances2 = ptr;

    ptr = FindSignature(base, size, patchFogSig);
    if (ptr)
        address.patchFog = ptr;

    ptr = FindSignature(base, size, patchForceMaxLodSig);
    if (ptr)
        address.patchForceMaxLod = ptr;
    
    ptr = FindSignature(base, size, patchRemoveItemFreezingSig);
    if (ptr)
        address.patchRemoveItemFreezing = ptr;

    ptr = FindSignature(base, size, patchDefaultFarValueSig1);
    if (ptr)
        address.patchDefaultFarValue1 = ptr;

    ptr = FindSignature(base, size, patchDefaultFarValueSig2);
    if (ptr)
        address.patchDefaultFarValue2 = ptr;

    ptr = FindSignature(base, size, defaultFarValueSig);
    if (ptr)
        address.defaultFarValue = ptr + 0x2;
    
    ptr = FindSignature(base, size, patchRemoveCullingSig1);
    if (ptr)
        address.patchRemoveCulling1 = ptr;

    ptr = FindSignature(base, size, patchRemoveCullingSig2);
    if (ptr)
        address.patchRemoveCulling2 = ptr;

    ptr = FindSignature(base, size, patchRemoveCullingSig3);
    if (ptr)
        address.patchRemoveCulling3 = ptr;

    ptr = FindSignature(base, size, patchRemoveCullingSig4);
    if (ptr)
        address.patchRemoveCulling4 = ptr;

    ptr = FindSignature(base, size, patchRemoveCullingSig5);
    if (ptr)
        address.patchRemoveCulling5 = ptr;

    ptr = FindSignature(base, size, patchRemoveCullingSig6);
    if (ptr)
        address.patchRemoveCulling6 = ptr + 0xA;

    ptr = FindSignature(base, size, patchNoBonksSig);
    if (ptr)
        address.patchNoBonks = ptr;

    ptr = FindSignature(base, size, levelIdBaseSig);
    if (ptr)
        address.levelIdBase = ReadPtr32(ptr + 0x2);

    ptr = FindSignature(base, size, getIDSig);
    if (ptr)
        address.getID = ptr;

    ptr = FindSignature(base, size, playerObjectsSig);
    if (ptr)
        address.playerObjects = ReadPtr32(ptr + 0x2);

    ptr = FindSignature(base, size, systemDatasPtrSig);
    if (ptr)
        address.systemDatasPtr = ReadPtr32(ptr + 0x2);

    ptr = FindSignature(base, size, rdrPtrSig);
    if (ptr)
        address.rdrPtr = ReadPtr32(ptr + 0x1);
    
    ptr = FindSignature(base, size, classMgrPtrSig);
    if (ptr)
        address.classMgrPtr = ReadPtr32(ptr + 0x2);

    ptr = FindSignature(base, size, getFontPtrSig);
    if (ptr)
        address.getFontPtr = ptr;
    
    ptr = FindSignature(base, size, drawStringFnSig);
    if (ptr)
        address.drawStringFn = ptr;
    
    ptr = FindSignature(base, size, patchDrawFpsSig);
    if (ptr)
        address.patchDrawFps = ptr;
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

extern "C" {
    uintptr_t hClipCursorAddress;
    uintptr_t hFpsFix1Addr;
    uintptr_t jmpBackAddressCursor;
    uintptr_t jmpBackAddressSetWindowPosPush;
    uintptr_t jmpBackAddressConsoleEnable;
    uintptr_t jmpBackAddressShowConsole;
    uintptr_t jmpBackAddressFpsFix1;
    uintptr_t jmpBackAddressFpsFix2;
    uintptr_t jmpBackAddressDeferredCasterDraw;
    uintptr_t cleanUpFpsFix2;
}

void __declspec(naked) hClipCursor() {
    __asm {
        call GetClientRect
        lea ecx, [esp+0x1c]
        mov eax, [hClipCursorAddress]
        mov edx, [eax]
        push 0x2
        push ecx
        push 0x0
        push edx
        call MapWindowPoints
        jmp jmpBackAddressCursor
    }
}

void __declspec(naked) hSetWindowPosPushBL() {
    __asm {
        push 0x0
        sub eax,0x14
        sub ebp,0x6
        push eax
        push ebp
        jmp jmpBackAddressSetWindowPosPush
    }
}

void __declspec(naked) hSetWindowPosPushWND() {
    __asm {
        push 0x0
        add eax,0x9
        push eax
        push ebp
        jmp jmpBackAddressSetWindowPosPush
    }
}

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
        jmp jmpBackAddressConsoleEnable
    }
}

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
        jmp jmpBackAddressShowConsole
    }
}


void __declspec(naked) hFpsFix1() {
    __asm {
        mov eax, [hFpsFix1Addr]
        add dword ptr[eax], 0x1
        push 0x1
        call timeBeginPeriod
        jmp jmpBackAddressFpsFix1
    }
}

void __declspec(naked) hFpsFix2() {
    __asm {
        mov eax, [cleanUpFpsFix2]
        call eax
        push 0x1
        call timeEndPeriod
        jmp jmpBackAddressFpsFix2
    }
}

// Changes PushDo(2) into PushDo((objectFlags & 0x100) ? 10 : 2),
// deferring the normal color draw of shadow-casting meshes.
void __declspec(naked) hDeferredCasterDrawOrder() {
    __asm {
        mov eax, [esp + 0x14]
        test dword ptr [eax + 0x68], 0x100
        mov eax, 0x2
        jz use_order
        mov eax, 0x0A
        use_order:
        push eax
        mov eax, [esi]
        mov edx, [eax + 0x88]
        mov ecx, esi
        jmp [jmpBackAddressDeferredCasterDraw]
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

    MH_STATUS hookStatus = MH_CreateHook(pEndScene, &hkEndScene, reinterpret_cast<void**>(&oEndScene));

    pDevice->Release();
    pD3D->Release();
    DestroyWindow(hWnd);
    UnregisterClassA("DummyWindow", wc.hInstance);

    return hookStatus == MH_OK;
}

void patch(BYTE* ptr, BYTE* buf, size_t len) {
    DWORD curProtection;
    VirtualProtect(ptr, len, PAGE_EXECUTE_READWRITE, &curProtection);
    memcpy(ptr, buf, len);
    VirtualProtect(ptr, len, curProtection, &curProtection);
}

void applyBasePatches(const RatataRConfig& cfg) {
    BYTE zero[] = {0x00,0x00,0x00,0x00};
    BYTE jmp[] = {0xEB};
    BYTE nop[] = {0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90};
    BYTE patchWindowed[] = {0xfe,0x85};
    BYTE patchWindowPosBorder[] = {0xfe,0xff,0xff,0xff};
    BYTE patchBorderless[] = {0x0a};
    BYTE patchWindowKey[] = {0x06};

    //Cursor only gets hidden when inside client area
    patch((BYTE*)addresses.patchCursorHide, CursorHidePatchBytes.data(), CursorHidePatchBytes.size());
    //Setting custom res
    if (cfg.displayMode != DisplayModes::Fullscreen) {
        patch((BYTE*)addresses.customResPatch1, zero, 1);

        auto screenW = static_cast<uint16_t>(cfg.width != 0 ? cfg.width : GetSystemMetrics(SM_CXSCREEN));
        auto screenH = static_cast<uint16_t>(cfg.height != 0 ? cfg.height : GetSystemMetrics(SM_CYSCREEN));
        patch((BYTE*)addresses.customResPatch2, reinterpret_cast<BYTE*>(&screenW), 2);
        patch((BYTE*)addresses.customResPatch3, reinterpret_cast<BYTE*>(&screenH), 2);
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

    patch((BYTE*)addresses.musicVideoDirectory, (BYTE*)reinterpret_cast<const BYTE*>(rootDirectory.c_str()), rootDirectory.size() + 1);

    // Stop directory from being overwritten
    // This one is scary because I don't exactly know what the purpose if this one is.
    // Please check this when you have the time.
    patch((BYTE*)addresses.stopMusicVideoDirectoryOverwrite, nop, 6);

    //Allow banned save names
    patch((BYTE*)addresses.patchAllowBannedSaveNames, jmp, 1);

    //Allow entering blank save name
    patch((BYTE*)addresses.patchAllowEmptySaveNames, jmp, 1);
    
    //Allow multiple game instances
    if (addresses.patchAllowMultiInstances1 && addresses.patchAllowMultiInstances2) {
        patch((BYTE*)addresses.patchAllowMultiInstances1, jmp, 1);
        patch((BYTE*)addresses.patchAllowMultiInstances2, nop, 2);
    }

    if (cfg.invertVerticalLook) {
        patch((BYTE*)addresses.patchInvertVerticalLook, (BYTE*)"\x75", 1);
    }

    if (!cfg.autoSave) {
        patch((BYTE*)addresses.patchAutoSave, nop, 13);
    }
}

void applyNonSpeedrunPatches(const RatataRConfig& cfg) {
    // These patches will not get applied when speedrun mode is turned on!
    
    //Apply FOV
    patch((BYTE*)addresses.bypassFovOverflow, (BYTE*)"\xEB", 1); //BYPASS FOV OVERFLOW ERROR

    auto fovVal = getFOVValues(cfg.fov);
    patch((BYTE*)addresses.fov, reinterpret_cast<BYTE*>(&fovVal.normal), 4);
    patch((BYTE*)addresses.climbFov, reinterpret_cast<BYTE*>(&fovVal.climbing), 4);
    patch((BYTE*)addresses.runSlideFov, reinterpret_cast<BYTE*>(&fovVal.runningSliding), 8);

    if (cfg.improvedViewDistance) {
        //Use default far value
        patch((BYTE*)addresses.patchDefaultFarValue1, (BYTE*)"\x90\x90", 2);
        patch((BYTE*)addresses.patchDefaultFarValue2, (BYTE*)"\x90\x90", 2);

        //Set default far value to 500
        float defaultFar = 500.0f;
        patch((BYTE*)addresses.defaultFarValue, reinterpret_cast<BYTE*>(&defaultFar), 4);
        
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

void applyBugFixes(const RatataRConfig& cfg) {
    if (!cfg.bugFixes)
        return;

    if (cfg.deferredShadowCasterDraw) {
        constexpr size_t hookLengthDeferredCasterDraw = 10;
        jmpBackAddressDeferredCasterDraw = static_cast<DWORD>(
            addresses.hookAddressDeferredCasterDraw + hookLengthDeferredCasterDraw
        );

        hook(
            reinterpret_cast<void*>(addresses.hookAddressDeferredCasterDraw),
            hDeferredCasterDrawOrder,
            hookLengthDeferredCasterDraw
        );
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

void __fastcall DrawFps() {
    auto fonts = follow_pointer_chain<DynArray_Z<hdl>>(systemDatasPtr, {0x18});
    if (fonts == nullptr)
        return;

    if (fonts->GetSize() == 0)
        return;

    char* font = getPtr(*classMgrPtr, 0, &fonts->Get(0));
    if (font == nullptr) {
        return;
    }

    auto fpsPtr = follow_pointer_chain<float>(rdrPtr, {0xc34});
    if (fpsPtr == nullptr)
        return;

    char fpsString[32];
    _snprintf_s(
        fpsString,
        sizeof(fpsString),
        _TRUNCATE,
        "%.0f",
        *fpsPtr
    );

    constexpr Color purple = { 0.4f, 0.1f, 1.0f, 1.0f };
    constexpr Color otherPurple = { 0.1f, 0.1f, 1.0f, 1.0f };
    constexpr Color black = { 0.0f, 0.0f, 0.0f, 1.0f };
    constexpr Vec2f position = { 4.0f, -2.0f };

    FontParam_Z fontParam{
        .text = fpsString,
        .hasBorder = true,
        .borderOffset = 2.0f,
        .borderColor = black,
        .topBoundY = -1.0f,
        .bottomBoundY = -1.0f,
        .position = position,
        .bottomColor = purple,
        .topColor = otherPurple,
        .scale1 = 0.8f,
        .scale2 = 1.0f,
        .zOffset = 0.f
    };

    drawStringFn(font, 0, &fontParam);
}

void ApplyHooks(const RatataRConfig& cfg) {
    const HookEntry hooks[] = {
        {addresses.hookAddressCursor, 6, hClipCursor, &jmpBackAddressCursor},
        {addresses.hookAddressSetWindowPosPush, 6, hSetWindowPosPushBL, &jmpBackAddressSetWindowPosPush, cfg.displayMode == DisplayModes::Borderless},
        {addresses.hookAddressSetWindowPosPush, 6, hSetWindowPosPushWND, &jmpBackAddressSetWindowPosPush, cfg.displayMode == DisplayModes::Windowed},
        {addresses.hookAddressConsoleEnable, 6, hEnableConsole, &jmpBackAddressConsoleEnable, cfg.console},
        {addresses.hookAddressShowConsole, 6, hShowConsole, &jmpBackAddressShowConsole, cfg.console},
        {addresses.hookAddressFpsFix1, 7, hFpsFix1, &jmpBackAddressFpsFix1},
        {addresses.hookAddressFpsFix2, 5, hFpsFix2, &jmpBackAddressFpsFix2}
    };

    for (auto& h : hooks) {
        if (!h.condition)
            continue;

        *h.trampoline = h.address + h.length;
        hook(reinterpret_cast<void*>(h.address), h.detour, h.length);
    }

    hClipCursorAddress = addresses.hClipCursor;
    hFpsFix1Addr = addresses.hFpsFix1;
    cleanUpFpsFix2 = addresses.hFpsFix2CleanUp;
    levelIdBase = addresses.levelIdBase;
    playerObjectsBase = addresses.playerObjects;
    getIDBase = addresses.getID;

    systemDatasPtr = reinterpret_cast<char**>(addresses.systemDatasPtr);
    rdrPtr = reinterpret_cast<char**>(addresses.rdrPtr);
    classMgrPtr = reinterpret_cast<char**>(addresses.classMgrPtr);
    getPtr = reinterpret_cast<char*(__fastcall*)(char*, int, hdl*)>(addresses.getFontPtr);
    drawStringFn = reinterpret_cast<char*(__fastcall*)(char*, int, FontParam_Z*)>(addresses.drawStringFn);

    if (cfg.displayFrameCounter) {
        PatchCall32((void*)addresses.patchDrawFps, (void*)&DrawFps);
    }

    MH_Initialize();
    HookD3D9EndScene();
    MH_EnableHook(MH_ALL_HOOKS);
}

bool ValidateSigScan(const PatchAddresses& addrStruct) {
    constexpr std::array<size_t, 2> sigWhitelist {
        offsetof(PatchAddresses, patchAllowMultiInstances1) / sizeof(PatchAddresses::patchAllowMultiInstances1),
        offsetof(PatchAddresses, patchAllowMultiInstances2) / sizeof(PatchAddresses::patchAllowMultiInstances2)
    };

    constexpr size_t addrCount = sizeof(PatchAddresses) / sizeof(uintptr_t);
    const uintptr_t* values = reinterpret_cast<const uintptr_t*>(&addrStruct);
    
    for (size_t i = 0; i < addrCount; i++) {
        bool safeToIgnore = std::find(sigWhitelist.begin(), sigWhitelist.end(), i) != sigWhitelist.end();
        if (safeToIgnore)
            continue;
        
        if (values[i] == 0) {
            char buffer[256];
            sprintf_s(buffer, sizeof(buffer), "Could not find signature %lu! The game will run without patches.", i + 1);
            MessageBoxA(nullptr, buffer, "RatataR", MB_OK);
            return false;
        }
    }

    return true;
}

DWORD WINAPI HookMain(LPVOID param) {
    HANDLE readyEvent = CreateEventA(nullptr, TRUE, FALSE, "RatataR_Patched");
    if (!readyEvent) {
        TerminateProcess(GetCurrentProcess(), 0);
        return 1;
    }

    TCHAR moduleFileName[MAX_PATH];
    GetModuleFileNameA((HMODULE)param, (LPSTR)moduleFileName, MAX_PATH);
    std::string::size_type pos = std::string((char*)moduleFileName).find_last_of("\\/");
    std::string configPath = std::string((char*)moduleFileName).substr(0, pos).append("\\").append("RatataRconfig.ini");
    rootDirectory = std::string((char*)moduleFileName).substr(0, pos);

    getSignatures(addresses);

    if (!ValidateSigScan(addresses)) {
        SetEvent(readyEvent);
        CloseHandle(readyEvent);
        return 0;
    }

    ConfigHandler cfgHandler;
    cfgHandler.load(configPath);
    const RatataRConfig config = cfgHandler.getConfig();

    g_EnableFrameLimit = config.maxFps > 0.0;
    g_TargetFPS = static_cast<double>(config.maxFps);

    applyBasePatches(config);
    if (!config.speedrunMode) {
        applyNonSpeedrunPatches(config);
        applyBugFixes(config);
    }
    ApplyHooks(config);

    if (config.discordRichPresence && InitRPC())
        CreateThread(nullptr, 0, RPCThreadMain, nullptr, 0, nullptr);

    SetEvent(readyEvent);
    CloseHandle(readyEvent);

    return 0;
}
