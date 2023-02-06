#include <Windows.h>
#include<iostream>

unsigned int height;
unsigned int width;
bool borderless;
bool windowed;
bool console;
BYTE* resW;
BYTE* resH;

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

DWORD jmpBackAddressShowConsole;
void __declspec(naked) hShowConsole() {
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
        jmp [jmpBackAddressShowConsole]
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

void applyPatches(LPVOID param) {
    BYTE zero[] = {0x00};
    BYTE patchCursorHide[] = {0x66,0x81,0x7c,0x24,0x18,0x01,0x00,0x75};
    BYTE patchWindowed[] = {0xfe,0x85};
    BYTE patchWindowPosBorder[] = {0xfe,0xff,0xff,0xff};
    BYTE patchWindowPosBorderless[] = {0x00,0x00,0x00,0x00};
    BYTE patchBorderless[] = {0x0a};
    BYTE patchWindowKey[] = {0x06};
    BYTE patchConsole0[] = {0xEB};
    BYTE patchConsole1[] = {0x8B, 0x0D, 0xA0, 0xE8, 0x7D, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90};
    BYTE patchConsole2[] = {0x1F, 0xBE, 0x18};
    BYTE patchConsole3[] = {0x90, 0x90, 0x90};

    //Cursor only gets hidden when inside client area
    patch((BYTE*)0x654711, patchCursorHide, 8);
    //Setting custom res
    if (windowed) {
        patch((BYTE*)0x657e58, zero, 1);
        patch((BYTE*)0x657e5e, resW, 2);
        patch((BYTE*)0x657e65, resH, 2);
    }
    //Initial window position
    patch((BYTE*)0x658d00, zero, 1);
    patch((BYTE*)0x658d05, zero, 1);
    //Not showing window before d3d9 device is created
    patch((BYTE*)0x658d0a, zero, 1);
    //Make game run windowed
    if (windowed) {
        patch((BYTE*)0x674750, patchWindowed, 2);
    }
    //Set window position to top left corner
    patch((BYTE*)0x67a9d9, borderless ? patchWindowPosBorderless : patchWindowPosBorder, 4);
    if (windowed && borderless) {
        patch((BYTE*)0x67a9ef, patchBorderless, 1);
    }
    //Change directinput flag so windows key works
    patch((BYTE*)0x6a91c6, patchWindowKey, 1);

    //Enable console
    if (console) {
        patch((BYTE*)0x004DE8E7, patchConsole0, 1);
        patch((BYTE*)0x004DE900, patchConsole1, 12);
        patch((BYTE*)0x004DE90D, patchConsole2, 3);
        patch((BYTE*)0x004DE913, patchConsole3, 3);
        patch((BYTE*)0x0066A2E4, patchConsole3, 3);
    }
}

DWORD WINAPI MainThread(LPVOID param) {
    TCHAR moduleFileName[MAX_PATH];
    GetModuleFileNameA((HMODULE)param, (LPSTR)moduleFileName, MAX_PATH);
    std::string::size_type pos = std::string((char*)moduleFileName).find_last_of("\\/");
    std::string configPath = std::string((char*)moduleFileName).substr(0, pos).append("\\").append("taRconfig.ini");
    width = GetPrivateProfileIntA("CONFIG","width",800,configPath.c_str());
    height = GetPrivateProfileIntA("CONFIG","height",600,configPath.c_str());
    windowed = GetPrivateProfileIntA("CONFIG","windowed",0,configPath.c_str());
    borderless = GetPrivateProfileIntA("CONFIG","borderless",0,configPath.c_str());
    console = GetPrivateProfileIntA("CONFIG","console",0,configPath.c_str());
    resW = reinterpret_cast<BYTE*>(&width);
    resH = reinterpret_cast<BYTE*>(&height);

    DWORD hookAddressCursor = 0x670991;
    DWORD hookAddressSetWindowPosPush = 0x67a9d0;
    DWORD hookAddressConsoleEnable = 0x66a2e7;
    size_t hookLengthCursor = 6;
    size_t hookLengthSetWindowPosPush = 6;
    size_t hookLengthConsoleEnable = 6;
    jmpBackAddressCursor = hookAddressCursor+hookLengthCursor;
    jmpBackAddressSetWindowPosPush = hookAddressSetWindowPosPush+hookLengthSetWindowPosPush;
    jmpBackAddressShowConsole = hookAddressConsoleEnable+hookLengthConsoleEnable;
    applyPatches(param);
    hook((void*)hookAddressCursor,hClipCursor,hookLengthCursor);
    if (windowed && borderless) {
        hook((void*)hookAddressSetWindowPosPush, hSetWindowPosPushBL, hookLengthSetWindowPosPush);
    } else if (windowed) {
        hook((void*)hookAddressSetWindowPosPush, hSetWindowPosPushWND, hookLengthSetWindowPosPush);
    }
    if (console) {
        hook((void*)hookAddressConsoleEnable,hShowConsole,hookLengthConsoleEnable);
    }
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