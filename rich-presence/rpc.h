#pragma once
#include <Windows.h>
#include "include/rpc-client/discord_manager.h"

DWORD WINAPI InitRPC(LPVOID lpParameter) {
    DiscordMan_Startup();
    Sleep(10000);
    while (true) {
		DiscordMan_Update();
		Sleep(10);
	}
    return TRUE;
}
