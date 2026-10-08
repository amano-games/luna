#pragma once

#include "base/context-cracking.h"
#include "base/types.h"
#include "base/dbg.h"

#if OS_WINDOWS
#define SYS_STEAM_CALL __cdecl
#else
#define SYS_STEAM_CALL
#endif

// Steam C API types and callback layouts.
enum {
	SYS_STEAM_ERR_MSG_MAX = 1024,
};

// steam_api.h ESteamAPIInitResult.
enum sys_steam_init_result {
	SYS_STEAM_INIT_OK               = 0,
	SYS_STEAM_INIT_FAILED_GENERIC   = 1,
	SYS_STEAM_INIT_NO_CLIENT        = 2,
	SYS_STEAM_INIT_VERSION_MISMATCH = 3,
};

typedef char sys_steam_errmsg[SYS_STEAM_ERR_MSG_MAX];

enum sys_steam_reset_achievements {
	SYS_STEAM_RESET_STATS_ONLY,
	SYS_STEAM_RESET_WITH_ACHIEVEMENTS,
};

enum {
	SYS_STEAM_SCOREBOARD_FIND_DATA_SIZE = 1104,
	SYS_STEAM_SCOREBOARD_GET_DATA_SIZE  = 1105,
	SYS_STEAM_SCOREBOARD_ADD_DATA_SIZE  = 1106,
};

enum sys_steam_scores_data_req {
	SYS_STEAM_SCORES_DATA_GLOBAL = 0,
};

enum sys_steam_scores_upload_method {
	SYS_STEAM_SCORES_UPLOAD_KEEP_BEST = 1,
};

typedef struct ISteamUserStats ISteamUserStats;
typedef struct ISteamUtils ISteamUtils;
typedef struct ISteamFriends ISteamFriends;
typedef struct ISteamUser ISteamUser;

typedef u64 sys_steam_api_call;
typedef u64 sys_steam_board_handle;
typedef u64 sys_steam_scoreboard_entries;
typedef u64 sys_steam_id;

// Valve callback packing: Linux/macOS SMALL (4), Windows LARGE (8).
#if OS_WINDOWS
#pragma pack(push, 8)
#else
#pragma pack(push, 4)
#endif

struct sys_steam_scoreboard_find {
	sys_steam_board_handle board_handle;
	b8 found;
};

struct sys_steam_scoreboard_get {
	sys_steam_board_handle board_handle;
	sys_steam_scoreboard_entries entries;
	i32 count;
};

struct sys_steam_scoreboard_add {
	u8 success;
	sys_steam_board_handle board_handle;
	i32 score;
	u8 changed;
	i32 rank_new;
	i32 rank_prev;
};

struct sys_steam_scoreboard_entry {
	sys_steam_id steam_id;
	i32 global_rank;
	i32 score;
	i32 details;
	u64 ugc;
};

#pragma pack(pop)

#if OS_WINDOWS
dbg_static_assert(sizeof(struct sys_steam_scoreboard_find) == 16, steam_scoreboard_find_sz);
dbg_static_assert(sizeof(struct sys_steam_scoreboard_downloaded) == 24, steam_scoreboard_dl_sz);
dbg_static_assert(sizeof(struct sys_steam_scoreboard_uploaded) == 32, steam_scoreboard_up_sz);
dbg_static_assert(sizeof(struct sys_steam_scoreboard_entry) == 32, steam_scoreboard_entry_sz);
#else
dbg_static_assert(sizeof(struct sys_steam_scoreboard_find) == 12, steam_lb_find_sz);
dbg_static_assert(sizeof(struct sys_steam_scoreboard_get) == 20, steam_lb_dl_sz);
dbg_static_assert(sizeof(struct sys_steam_scoreboard_add) == 28, steam_lb_up_sz);
dbg_static_assert(sizeof(struct sys_steam_scoreboard_entry) == 28, steam_lb_entry_sz);
#endif

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
	sys_steam_board_handle leaderboard,
	enum sys_steam_scores_data_req data_req,
	i32 range_start,
	i32 range_end);
sys_steam_api_call SYS_STEAM_CALL SteamAPI_ISteamUserStats_DownloadLeaderboardEntriesForUsers(
	ISteamUserStats *self,
	sys_steam_board_handle leaderboard,
	sys_steam_id *users,
	i32 user_count);
b32 SYS_STEAM_CALL SteamAPI_ISteamUserStats_GetDownloadedLeaderboardEntry(
	ISteamUserStats *self,
	sys_steam_scoreboard_entries entries,
	i32 index,
	struct sys_steam_scoreboard_entry *entry,
	i32 *details,
	i32 details_max);
sys_steam_api_call SYS_STEAM_CALL SteamAPI_ISteamUserStats_UploadLeaderboardScore(
	ISteamUserStats *self,
	sys_steam_board_handle leaderboard,
	enum sys_steam_scores_upload_method method,
	i32 score,
	const i32 *score_details,
	i32 score_details_count);
// NOLINTEND(readability-identifier-naming)
