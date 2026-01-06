#pragma once
#include <Windows.h>
#include "include/rpc-client/discord_manager.h"

constexpr DWORD UpdateFreq = 1000;

DWORD WINAPI InitRPC(LPVOID lpParameter) {
    DiscordMan_Startup();

    while (true) {
		DiscordMan_Update();
		Sleep(UpdateFreq);
	}

    return TRUE;
}
