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

enum sys_steam_reset_achievements {
	SYS_STEAM_RESET_STATS_ONLY,
	SYS_STEAM_RESET_WITH_ACHIEVEMENTS,
};

enum {
	SYS_STEAM_CB_LB_FIND       = 1104,
	SYS_STEAM_CB_LB_DOWNLOADED = 1105,
	SYS_STEAM_CB_LB_UPLOADED   = 1106,
};

enum sys_scores_lb_data_req {
	SYS_SCORES_LB_DATA_GLOBAL = 0,
};

enum sys_scores_lb_upload_method {
	SYS_SCORES_LB_UPLOAD_KEEP_BEST = 1,
};

typedef struct ISteamUserStats ISteamUserStats;
typedef struct ISteamUtils ISteamUtils;
typedef struct ISteamFriends ISteamFriends;
typedef struct ISteamUser ISteamUser;

typedef u64 sys_steam_api_call;
typedef u64 sys_steam_leaderboard;
typedef u64 sys_steam_lb_entries;
typedef u64 sys_steam_id;

// Valve callback packing: Linux/macOS SMALL (4), Windows LARGE (8).
#if OS_WINDOWS
#pragma pack(push, 8)
#else
#pragma pack(push, 4)
#endif

struct sys_steam_lb_find {
	sys_steam_leaderboard leaderboard;
	u8 found;
};

struct sys_steam_lb_downloaded {
	sys_steam_leaderboard leaderboard;
	sys_steam_lb_entries entries;
	i32 count;
};

struct sys_steam_lb_uploaded {
	u8 success;
	sys_steam_leaderboard leaderboard;
	i32 score;
	u8 changed;
	i32 rank_new;
	i32 rank_prev;
};

struct sys_steam_lb_entry {
	sys_steam_id steam_id;
	i32 global_rank;
	i32 score;
	i32 details;
	u64 ugc;
};

#pragma pack(pop)

#if OS_WINDOWS
dbg_static_assert(sizeof(struct sys_steam_lb_find) == 16, steam_lb_find_sz);
dbg_static_assert(sizeof(struct sys_steam_lb_downloaded) == 24, steam_lb_dl_sz);
dbg_static_assert(sizeof(struct sys_steam_lb_uploaded) == 32, steam_lb_up_sz);
dbg_static_assert(sizeof(struct sys_steam_lb_entry) == 32, steam_lb_entry_sz);
#else
dbg_static_assert(sizeof(struct sys_steam_lb_find) == 12, steam_lb_find_sz);
dbg_static_assert(sizeof(struct sys_steam_lb_downloaded) == 20, steam_lb_dl_sz);
dbg_static_assert(sizeof(struct sys_steam_lb_uploaded) == 28, steam_lb_up_sz);
dbg_static_assert(sizeof(struct sys_steam_lb_entry) == 28, steam_lb_entry_sz);
#endif
