#include <iostream>
#include <Windows.h>
#include <string>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    const char* dllName = "hook.dll";
    const char* processName = "overlay.exe";

    char moduleFileName[MAX_PATH];
    GetModuleFileNameA(nullptr, moduleFileName, MAX_PATH);

    std::string basePath = moduleFileName;
    size_t pos = basePath.find_last_of("\\/");
    basePath = basePath.substr(0, pos);

    std::string overlayPath = basePath + "\\" + processName;
    std::string dllPath = basePath + "\\" + dllName;

    STARTUPINFO si{};
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi{};
    if (!CreateProcessA(
        overlayPath.c_str(),
        nullptr,
        nullptr,
        nullptr,
        FALSE,
        CREATE_SUSPENDED,
        nullptr,
        nullptr,
        &si,
        &pi
    )) {
        MessageBoxA(nullptr, "Unable to launch Ratatouille.", "RatataR", MB_OK | MB_ICONERROR);
        return 1;
    }

    LPVOID remoteMem = VirtualAllocEx(
        pi.hProcess,
        nullptr,
        dllPath.size() + 1,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE
    );

    if (!remoteMem) {
        MessageBoxA(nullptr, "VirtualAllocEx failed.", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    WriteProcessMemory(
        pi.hProcess,
        remoteMem,
        dllPath.c_str(),
        dllPath.length() + 1,
        nullptr
    );

    HANDLE hThread = CreateRemoteThread(
        pi.hProcess,
        nullptr,
        0,
        (LPTHREAD_START_ROUTINE)LoadLibraryA,
        remoteMem,
        0,
        nullptr
    );

    if (!hThread) {
        MessageBoxA(nullptr, "CreateRemoteThread failed.", "Error", MB_OK | MB_ICONERROR);
        VirtualFreeEx(pi.hProcess, remoteMem, 0, MEM_RELEASE);
        TerminateProcess(pi.hProcess, 0);
        return 1;
    }
    
    WaitForSingleObject(hThread, INFINITE);

    CloseHandle(hThread);
    VirtualFreeEx(pi.hProcess, remoteMem, 0, MEM_RELEASE);

    HANDLE evt = OpenEventA(SYNCHRONIZE, FALSE, "RatataR_Patched");
    if (evt) {
        WaitForSingleObject(evt, INFINITE);
        CloseHandle(evt);
    }

    ResumeThread(pi.hThread);

    HWND hWnd = FindWindowA(NULL, "Ratatouille");
    if (hWnd) {
        SetForegroundWindow(hWnd);
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return 0;
}