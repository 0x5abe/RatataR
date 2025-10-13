// ================================================== \\
// Discord RPC implemenatation
// Based on code from the Valve Dev Community wiki
// 
// Jay - 2022
// ================================================== \\

#include "discord_manager.h"
#include <string.h>
#include "discord-rpc/include/discord_rpc.h"
#include <time.h>

static DiscordRichPresence discordPresence;

// Blank handlers; not required for singleplayer Half-Life
static void HandleDiscordReady(const DiscordUser* connectedUser) {}
static void HandleDiscordDisconnected(int errcode, const char* message) {}
static void HandleDiscordError(int errcode, const char* message) {}
static void HandleDiscordJoin(const char* secret) {}
static void HandleDiscordSpectate(const char* secret) {}
static void HandleDiscordJoinRequest(const DiscordUser* request) {}

// Default logo to use as a fallback
const char* defaultLogo = "IMAGE_NAME_HERE";

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

	Discord_Initialize("GAME_ID_HERE", &handlers, 1, 0);

	memset(&discordPresence, 0, sizeof(discordPresence));

	discordPresence.startTimestamp = startTime;
	discordPresence.largeImageKey = defaultLogo;
	Discord_UpdatePresence(&discordPresence);
}

void DiscordMan_Update(void)
{
	char curArea[64];	// If the CVar is empty, use the map file name
	char curImage[16];	// If the CVar is empty, use the default logo

	discordPresence.details = 1;	// Chapter name doesn't matter; if it's blank, Discord shows nothing
	discordPresence.state = curArea;
	discordPresence.largeImageKey = curImage;

	Discord_UpdatePresence(&discordPresence);
}

void DiscordMan_Kill(void)
{
	Discord_Shutdown();
}
