// ================================================== \\
// Discord RPC implemenatation
// 
// Jay - 2022
// ================================================== \\
//include "discord_manager.h"
#include <string.h>
#include <iostream>
#include <time.h>
#include "include/discord-rpc/include/discord_rpc.h"
#include "levels.h"
static DiscordRichPresence discordPresence;

// Blank handlers; not required for singleplayer Half-Life
static void HandleDiscordReady(const DiscordUser* connectedUser) {}
static void HandleDiscordDisconnected(int errcode, const char* message) {}
static void HandleDiscordError(int errcode, const char* message) {}
static void HandleDiscordJoin(const char* secret) {}
static void HandleDiscordSpectate(const char* secret) {}
static void HandleDiscordJoinRequest(const DiscordUser* request) {}
// Default logo to use as a fallback
const char* defaultLogo = "bootingup";

void DiscordMan_Startup(void)
{
	int64_t startTime = time(0);

	DiscordEventHandlers handlers;
	memset(&handlers, 0, sizeof(handlers));

	handlers.ready = HandleDiscordReady;
	handlers.disconnected = HandleDiscordDisconnected;
	handlers.errored = HandleDiscordError;
	handlers.joinGame = HandleDiscordJoin;
	handlers.spectateGame = HandleDiscordSpectate;
	handlers.joinRequest = HandleDiscordJoinRequest;

	Discord_Initialize("1219347301521162331", &handlers, 1, 0);

	memset(&discordPresence, 0, sizeof(discordPresence));

	discordPresence.startTimestamp = startTime;
	discordPresence.largeImageKey = defaultLogo;
	discordPresence.state = "In Menu";

	initRat();

	Discord_UpdatePresence(&discordPresence);
}
int pass = 0;
void DiscordMan_Update(void)
{
	char curArea[128];	// If the CVar is empty, use the map file name
	char curImage[16];	// If the CVar is empty, use the default logo

	sprintf_s(curImage, "bootingup");
	Level curLevel = getLevel();
	discordPresence.details = getCharName();
	discordPresence.state = curLevel.name;
	if (!pass) {
		discordPresence.largeImageKey = curImage;
		pass++;
	}
	else {
		discordPresence.largeImageKey = curLevel.imageName;
	}

	Discord_UpdatePresence(&discordPresence);
}

void DiscordMan_Kill(void)
{
	Discord_Shutdown();
}