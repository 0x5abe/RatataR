#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <string>

int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    const char* dllName = "hook.dll";
    const char* processName = "overlay.exe";
    const char* richPrecenseName = "discord-rpc.dll";

    char moduleFileName[MAX_PATH];
    GetModuleFileNameA(nullptr, moduleFileName, MAX_PATH);

    std::string basePath = moduleFileName;
    size_t pos = basePath.find_last_of("\\/");
    basePath = basePath.substr(0, pos);

    std::string overlayPath = basePath + "\\" + processName;
    std::string dllPath = basePath + "\\" + dllName;
    std::string richPresencePath = basePath + "\\" + richPrecenseName;

    STARTUPINFO si{};
    si.cb = sizeof(si);

    if (GetFileAttributesA(overlayPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        MessageBoxA(nullptr, "Failed to get game executable (overlay.exe).", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    if (GetFileAttributesA(dllPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        MessageBoxA(nullptr, "Failed to get hook.dll.", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    if (GetFileAttributesA(richPresencePath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        MessageBoxA(nullptr, "Failed to get discord-rpc.dll.", "Error", MB_OK | MB_ICONERROR);
        return 1;
    }

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

    DWORD threadExitCode = 0;
    DWORD waitResult = WaitForSingleObject(hThread, INFINITE);

    if (waitResult != WAIT_OBJECT_0) {
        MessageBoxA(nullptr, "Failed waiting for remote thread.", "Injection failed", MB_OK | MB_ICONERROR);
    }
    
    if (!GetExitCodeThread(hThread, &threadExitCode)) {
        MessageBoxA(nullptr, "Failed to get thread exit code.", "Error", MB_OK | MB_ICONERROR);
    } else if (threadExitCode == 0) {
        MessageBoxA(nullptr, "CreateRemoteThread failed for target process.", "Injection failed", MB_OK | MB_ICONERROR);
    }

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