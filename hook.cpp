#include <Windows.h>
#include <iostream>
#include <timeapi.h>

unsigned int height;
unsigned int width;
float fov;
float climbFOV;
double runSlideFOV;
bool console;
bool popupMenu;
bool invertVerticalLook;
bool removeFpsCap;
bool autoSave;
bool speedrunMode;
char buffer[256];
BYTE* resW;
BYTE* resH;
BYTE* noCDDirectory;
size_t noCDDirectorySize;
std::string displayMode;

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

void applyPatches(LPVOID param) {
    BYTE zero[] = {0x00};
    BYTE patchCursorHide[] = {0x66,0x81,0x7c,0x24,0x18,0x01,0x00,0x75};
    BYTE patchWindowed[] = {0xfe,0x85};
    BYTE patchWindowPosBorder[] = {0xfe,0xff,0xff,0xff};
    BYTE patchWindowPosBorderless[] = {0x00,0x00,0x00,0x00};
    BYTE patchBorderless[] = {0x0a};
    BYTE patchWindowKey[] = {0x06};
    BYTE patchConsole[] = {0x90, 0x90, 0x90};
    BYTE patchPopupMenu0[] = {0x90, 0x90};
    BYTE patchPopupMenu1[] = {0x90, 0x90, 0x90, 0x90, 0x90, 0x90};
    BYTE patchInvertVerticalLook[] = {0x01};
    BYTE patchRemoveFpsCap[] = {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90};

    //Cursor only gets hidden when inside client area
    patch((BYTE*)0x654711, patchCursorHide, 8);
    //Setting custom res
    if (displayMode != "fullscreen") {
        patch((BYTE*)0x657e58, zero, 1);
        patch((BYTE*)0x657e5e, resW, 2);
        patch((BYTE*)0x657e65, resH, 2);
    }
    //Initial window position
    patch((BYTE*)0x658d05, zero, 1);
    //Not showing window before d3d9 device is created
    patch((BYTE*)0x658d0a, zero, 1);
    patch((BYTE*)0x658d00, zero, 1);
    //Make game run windowed
    if (displayMode != "fullscreen") {
        patch((BYTE*)0x674750, patchWindowed, 2);
    }
    //Set window position to top left corner
    patch((BYTE*)0x67a9d9, displayMode == "borderless" ? patchWindowPosBorderless : patchWindowPosBorder, 4);
    if (displayMode == "borderless") {
        patch((BYTE*)0x67a9ef, patchBorderless, 1);
    }
    //Change directinput flag so windows key works
    patch((BYTE*)0x6a91c6, patchWindowKey, 1);

    //BYPASS DISC REQUIREMENT
    patch((BYTE*)0x00654F52, (BYTE*)"\x90\x90\x90\x90\x90\x90", 6);
    patch((BYTE*)0x00654FD7, (BYTE*)"\x90\x90", 2);
    patch((BYTE*)0x00655015, (BYTE*)"\xEB", 1);
    //MUSIC & VIDEO DIRECTORY
    patch((BYTE*)0x007DF135, noCDDirectory, noCDDirectorySize);
    patch((BYTE*)0x00654D8D, (BYTE*)"\x90\x90\x90\x90\x90\x90", 6);

    if (!speedrunMode) {
        //Apply FOV
        patch((BYTE*)0x00580C7C, (BYTE*)"\xEB", 1); //BYPASS FOV OVERFLOW ERROR
        patch((BYTE*)0x0070A7D8, reinterpret_cast<BYTE*>(&fov), sizeof(float));
        patch((BYTE*)0x0070A7A4, reinterpret_cast<BYTE*>(&climbFOV), sizeof(float));
        patch((BYTE*)0x0070A798, reinterpret_cast<BYTE*>(&runSlideFOV), sizeof(double));

        //Enable console
        if (console) {
            patch((BYTE*)0x0066A2E4, patchConsole, 3);
        }

        if (popupMenu) {
            patch((BYTE*)0x00670983, patchPopupMenu0, 2);
            patch((BYTE*)0x00674738, patchPopupMenu1, 6);
        }

        if (removeFpsCap) {
            patch((BYTE*)0x006589D7, patchRemoveFpsCap, 8);
        }
    }

    if (invertVerticalLook) {
        patch((BYTE*)0x00406DB0, patchInvertVerticalLook, 1);
    }

    if (!autoSave) {
        patch((BYTE*)0x00559A04, (BYTE*)"\x00", 1);
    }
}

DWORD WINAPI MainThread(LPVOID param) {
    TCHAR moduleFileName[MAX_PATH];
    GetModuleFileNameA((HMODULE)param, (LPSTR)moduleFileName, MAX_PATH);
    std::string::size_type pos = std::string((char*)moduleFileName).find_last_of("\\/");
    std::string rootDirectory = std::string((char*)moduleFileName).substr(0, pos);
    std::string configPath = std::string((char*)moduleFileName).substr(0, pos).append("\\").append("RatataRconfig.ini");
    GetPrivateProfileStringA("CONFIG","displayMode","fullscreen",buffer,sizeof(buffer),configPath.c_str());
    width = GetPrivateProfileIntA("CONFIG","width",0,configPath.c_str());
    height = GetPrivateProfileIntA("CONFIG","height",0,configPath.c_str());
    console = GetPrivateProfileIntA("CONFIG","console",0,configPath.c_str());
    popupMenu = GetPrivateProfileIntA("CONFIG","popupMenu",0,configPath.c_str());
    invertVerticalLook = GetPrivateProfileIntA("CONFIG","invertVerticalLook",0,configPath.c_str());
    removeFpsCap = GetPrivateProfileIntA("CONFIG","removeFpsCap",0,configPath.c_str());
    autoSave = GetPrivateProfileIntA("CONFIG","autoSave",1,configPath.c_str());
    fov = GetPrivateProfileIntA("CONFIG","fov",95,configPath.c_str());
    speedrunMode = GetPrivateProfileIntA("CONFIG","speedrunMode",0,configPath.c_str());

    displayMode = buffer;
    for (char& c : displayMode) {
        c = std::tolower(c);
    }
    if (displayMode != "windowed" && displayMode != "borderless" && displayMode != "fullscreen") {
        displayMode = "fullscreen";
    }

    if (fov < 1 || fov > 155) fov = 95;
    climbFOV = (110.0f / 95.0f) * fov;
    runSlideFOV = (110.0 / 95.0) * fov;

    noCDDirectory = new BYTE[rootDirectory.length()];
    noCDDirectorySize = rootDirectory.length();

    for (size_t i = 0; i < rootDirectory.length(); ++i) {
        noCDDirectory[i] = static_cast <BYTE>(rootDirectory[i]);
    }

    if (width == 0) {
        width = GetSystemMetrics(SM_CXSCREEN);
    }

    if (height == 0) {
        height = GetSystemMetrics(SM_CYSCREEN);
    }

    resW = reinterpret_cast<BYTE*>(&width);
    resH = reinterpret_cast<BYTE*>(&height);

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
    applyPatches(param);
    hook((void*)hookAddressCursor,hClipCursor,hookLengthCursor);
    if (displayMode == "borderless") {
        hook((void*)hookAddressSetWindowPosPush, hSetWindowPosPushBL, hookLengthSetWindowPosPush);
    }
    else if (displayMode != "fullscreen") {
        hook((void*)hookAddressSetWindowPosPush, hSetWindowPosPushWND, hookLengthSetWindowPosPush);
    }
    if (console) {
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
