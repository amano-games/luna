#include "sys/steam/sys-steam.h"

#include "sys/sys-log.h"
#include "base/str.h"

static b32 G_SYS_STEAM_OK;

b32
sys_steam_ini(void)
{
	b32 should_quit = false;

#if 0
	if(SteamAPI_RestartAppIfNecessary((u32)STEAM_APP_ID)) {
		log_info("steam", "restart via steam (app_id=%u)", (u32)STEAM_APP_ID);
		should_quit = true;
	} else
#endif
	{
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
#if SYS_SCORES_BACKEND == SYS_SCORES_STEAM
		sys_steam_scores_tick();
#endif
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

void
sys_steam_stats_reset(enum sys_steam_reset_achievements achievements)
{
	ISteamUserStats *user_stats = sys_steam_user_stats();
	if(user_stats) {
		b32 achievements_too = (achievements == SYS_STEAM_RESET_WITH_ACHIEVEMENTS);
		if(!SteamAPI_ISteamUserStats_ResetAllStats(user_stats, achievements_too)) {
			log_warn("steam", "ResetAllStats failed");
		}
	}
}

b32
sys_steam_ok(void)
{
	return G_SYS_STEAM_OK;
}
