#pragma once

#include "sys/steam/sys-steam-defs.h"

// NOLINTBEGIN(readability-identifier-naming)
b32 SYS_STEAM_CALL SteamAPI_RestartAppIfNecessary(u32 app_id);
enum sys_steam_init_result SYS_STEAM_CALL SteamAPI_InitFlat(sys_steam_errmsg *err_msg);
void SYS_STEAM_CALL SteamAPI_Shutdown(void);
void SYS_STEAM_CALL SteamAPI_RunCallbacks(void);

ISteamUserStats *SYS_STEAM_CALL SteamAPI_SteamUserStats_v013(void);
b32 SYS_STEAM_CALL SteamAPI_ISteamUserStats_SetAchievement(ISteamUserStats *self, const char *pch_name);
b32 SYS_STEAM_CALL SteamAPI_ISteamUserStats_SetStatInt32(ISteamUserStats *self, const char *pch_name, i32 n_data);
b32 SYS_STEAM_CALL SteamAPI_ISteamUserStats_StoreStats(ISteamUserStats *self);
b32 SYS_STEAM_CALL SteamAPI_ISteamUserStats_ResetAllStats(ISteamUserStats *self, b32 achievements_too);

ISteamUtils *SYS_STEAM_CALL SteamAPI_SteamUtils_v011(void);
ISteamFriends *SYS_STEAM_CALL SteamAPI_SteamFriends_v018(void);
ISteamUser *SYS_STEAM_CALL SteamAPI_SteamUser_v023(void);

sys_steam_id SYS_STEAM_CALL SteamAPI_ISteamUser_GetSteamID(ISteamUser *self);
const char *SYS_STEAM_CALL SteamAPI_ISteamFriends_GetFriendPersonaName(ISteamFriends *self, sys_steam_id steam_id);

b32 SYS_STEAM_CALL SteamAPI_ISteamUtils_IsAPICallCompleted(ISteamUtils *self, sys_steam_api_call call, bool *failed);
b32 SYS_STEAM_CALL SteamAPI_ISteamUtils_GetAPICallResult(
	ISteamUtils *self,
	sys_steam_api_call call,
	void *callback,
	i32 callback_size,
	i32 callback_expected,
	bool *failed);

sys_steam_api_call SYS_STEAM_CALL SteamAPI_ISteamUserStats_FindLeaderboard(ISteamUserStats *self, const char *name);
sys_steam_api_call SYS_STEAM_CALL SteamAPI_ISteamUserStats_DownloadLeaderboardEntries(
	ISteamUserStats *self,
	sys_steam_leaderboard leaderboard,
	enum sys_scores_lb_data_req data_req,
	i32 range_start,
	i32 range_end);
sys_steam_api_call SYS_STEAM_CALL SteamAPI_ISteamUserStats_DownloadLeaderboardEntriesForUsers(
	ISteamUserStats *self,
	sys_steam_leaderboard leaderboard,
	sys_steam_id *users,
	i32 user_count);
b32 SYS_STEAM_CALL SteamAPI_ISteamUserStats_GetDownloadedLeaderboardEntry(
	ISteamUserStats *self,
	sys_steam_lb_entries entries,
	i32 index,
	struct sys_steam_lb_entry *entry,
	i32 *details,
	i32 details_max);
sys_steam_api_call SYS_STEAM_CALL SteamAPI_ISteamUserStats_UploadLeaderboardScore(
	ISteamUserStats *self,
	sys_steam_leaderboard leaderboard,
	enum sys_scores_lb_upload_method method,
	i32 score,
	const i32 *score_details,
	i32 score_details_count);
// NOLINTEND(readability-identifier-naming)

b32 sys_steam_ini(void);
void sys_steam_tick(void);
void sys_steam_close(void);
b32 sys_steam_ok(void);

void sys_steam_achievement_unlock(str8 api_name);
void sys_steam_stat_set(str8 api_name, i32 value);
void sys_steam_stats_store(void);
void sys_steam_stats_reset(enum sys_steam_reset_achievements achievements);
