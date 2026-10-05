#include "sys/sys-steam.h"

#include "base/log.h"

static b32 G_SYS_STEAM_OK;

#if USE_STEAM

enum {
	SYS_STEAM_ERRMSG_MAX = 1024,
};

// steam_api.h ESteamAPIInitResult.
enum sys_steam_init_result {
	SYS_STEAM_INIT_OK               = 0,
	SYS_STEAM_INIT_FAILED_GENERIC   = 1,
	SYS_STEAM_INIT_NO_CLIENT        = 2,
	SYS_STEAM_INIT_VERSION_MISMATCH = 3,
};

typedef char sys_steam_errmsg[SYS_STEAM_ERRMSG_MAX];

#if OS_WINDOWS
#define SYS_STEAM_CALL __cdecl
#else
#define SYS_STEAM_CALL
#endif

// NOLINTBEGIN(readability-identifier-naming)
// Valve C exports
b32 SYS_STEAM_CALL SteamAPI_RestartAppIfNecessary(u32 app_id);
enum sys_steam_init_result SYS_STEAM_CALL SteamAPI_InitFlat(sys_steam_errmsg *err_msg);
void SYS_STEAM_CALL SteamAPI_Shutdown(void);
void SYS_STEAM_CALL SteamAPI_RunCallbacks(void);
// NOLINTEND(readability-identifier-naming)

b32
sys_steam_ini(void)
{
	b32 should_quit = false;

	if(SteamAPI_RestartAppIfNecessary((u32)STEAM_APP_ID)) {
		log_info("steam", "restart via steam (app_id=%u)", (u32)STEAM_APP_ID);
		should_quit = true;
	} else {
		sys_steam_errmsg err_msg          = {0};
		enum sys_steam_init_result result = SteamAPI_InitFlat(&err_msg);

		G_SYS_STEAM_OK = (result == SYS_STEAM_INIT_OK);
		if(G_SYS_STEAM_OK) {
			log_info("steam", "init ok (app_id=%u)", (u32)STEAM_APP_ID);
		} else {
			log_warn("steam", "init failed (app_id=%u, %d): %s", (u32)STEAM_APP_ID, (i32)result, err_msg);
		}
	}

	return should_quit;
}

void
sys_steam_tick(void)
{
	if(G_SYS_STEAM_OK) {
		SteamAPI_RunCallbacks();
	}
}

void
sys_steam_close(void)
{
	if(G_SYS_STEAM_OK) {
		SteamAPI_Shutdown();
		G_SYS_STEAM_OK = false;
	}
}

#else

b32
sys_steam_ini(void)
{
	return false;
}

void
sys_steam_tick(void)
{
}

void
sys_steam_close(void)
{
}

#endif

b32
sys_steam_ok(void)
{
	return G_SYS_STEAM_OK;
}
