#include "sys/sys-steam.h"

#include "base/log.h"
#include "base/str.h"

// #undef USE_STEAM
// #define USE_STEAM 1

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

typedef struct ISteamUserStats ISteamUserStats;

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

ISteamUserStats *SYS_STEAM_CALL SteamAPI_SteamUserStats_v013(void);
b32 SYS_STEAM_CALL SteamAPI_ISteamUserStats_SetAchievement(ISteamUserStats *self, const char *pch_name);
b32 SYS_STEAM_CALL SteamAPI_ISteamUserStats_SetStatInt32(ISteamUserStats *self, const char *pch_name, i32 n_data);
b32 SYS_STEAM_CALL SteamAPI_ISteamUserStats_StoreStats(ISteamUserStats *self);
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

static ISteamUserStats *
sys_steam_user_stats(void)
{
	ISteamUserStats *user_stats = NULL;
	if(G_SYS_STEAM_OK) {
		user_stats = SteamAPI_SteamUserStats_v013();
	}
	return user_stats;
}

void
sys_steam_achievement_unlock(str8 api_name)
{
	ISteamUserStats *user_stats = sys_steam_user_stats();
	if(user_stats && api_name.size && api_name.str[0] != '\0') {
		if(!SteamAPI_ISteamUserStats_SetAchievement(user_stats, (const char *)api_name.str)) {
			log_warn("steam", "SetAchievement failed: %s", api_name);
		}
	}
}

void
sys_steam_stat_set(str8 api_name, i32 value)
{
	ISteamUserStats *user_stats = sys_steam_user_stats();
	if(user_stats && api_name.size && api_name.str[0] != '\0') {
		if(!SteamAPI_ISteamUserStats_SetStatInt32(user_stats, (const char *)api_name.str, value)) {
			log_warn("steam", "SetStatInt32 failed: %.*s=%d", str8_spread(api_name), value);
		}
	}
}

void
sys_steam_stats_store(void)
{
	ISteamUserStats *user_stats = sys_steam_user_stats();
	if(user_stats) {
		if(!SteamAPI_ISteamUserStats_StoreStats(user_stats)) {
			log_warn("steam", "StoreStats failed");
		}
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

void
sys_steam_achievement_unlock(str8 api_name)
{
}

void
sys_steam_stat_set(str8 api_name, i32 value)
{
}

void
sys_steam_stats_store(void)
{
}

#endif

b32
sys_steam_ok(void)
{
	return G_SYS_STEAM_OK;
}
