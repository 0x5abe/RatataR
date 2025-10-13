#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    const char* dllName = "hook.dll";
    const char* processName = "overlay.exe";

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));

    HMODULE hModule = GetModuleHandleA(NULL);
    TCHAR moduleFileName[MAX_PATH];
    GetModuleFileNameA(hModule,(LPSTR)moduleFileName,MAX_PATH);
    std::string::size_type pos = std::string(moduleFileName).find_last_of("\\/");
    std::string overlayPath = std::string(moduleFileName).substr(0, pos).append("\\").append(processName);
    std::string dllPath = std::string(moduleFileName).substr(0, pos).append("\\").append(dllName);

    if (CreateProcessA((LPCSTR)overlayPath.c_str(),NULL,NULL,NULL,FALSE,CREATE_SUSPENDED,NULL,NULL,&si,&pi)) {
        if (pi.hProcess && pi.hProcess != INVALID_HANDLE_VALUE) { 
            void *loc = VirtualAllocEx(pi.hProcess, 0, MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
            if (loc) {
                WriteProcessMemory(pi.hProcess, loc, dllPath.c_str(), dllPath.length()+1, 0);
            }
            HANDLE hThread = CreateRemoteThread(pi.hProcess, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, loc, 0, 0);
            if (hThread) {
                WaitForSingleObject(hThread, INFINITE);
                CloseHandle(hThread);   
            }
            VirtualFreeEx(pi.hProcess, loc, dllPath.length()+1, MEM_RELEASE);
            
            ResumeThread(pi.hThread);
            
            HWND hWnd = FindWindowA(NULL,"Ratatouille");
            if (hWnd) {
                SetForegroundWindow(hWnd);
            }
        }
    }
    else {
        MessageBoxA(NULL, "Failed to initialize.", "RatataR", MB_OK | MB_ICONERROR);
    }
    return 0;
}