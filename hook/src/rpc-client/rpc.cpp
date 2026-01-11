#include <Windows.h>
#include "discord_manager.h"
#include "rpc.h"

constexpr DWORD UpdateFreq = 1000;

DWORD WINAPI InitRPC(LPVOID lpParameter) {
	DiscordMan_Startup();

	while (true) {
		DiscordMan_Update();
		Sleep(UpdateFreq);
	}

	return 0;
}
