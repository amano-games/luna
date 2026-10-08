#include "sys/sys-scoreboards.h"

#include "base/dbg.h"
#include "base/hash.h"
#include "base/ht.h"
#include "base/log.h"
#include "base/mem.h"
#include "base/str.h"
#include "base/types.h"
#include "base/utils.h"
#include "sys/sys.h"
#include "sys/steam/sys-steam.h"

#define SYS_SCORES_STEAM_SCORE_BIAS ((i64)I32_MAX + 1)

enum {
	SYS_SCORES_REQS_QUEUE_CAP        = 20,
	SYS_STEAM_SCORES_TOP_ENTRY_COUNT = 10,
	SYS_SCORES_LB_NAME_MAX           = 128,
	SYS_SCORES_LB_CACHE_CAP          = 8,
	SYS_SCORES_LB_CACHE_EXP          = 4,
};

enum sys_scores_req_type {
	SYS_SCORES_REQ_NONE,

	SYS_SCORES_REQ_GET,
	SYS_SCORES_REQ_ADD,
	SYS_SCORES_REQ_PERSONAL_BEST,

	SYS_SCORES_REQ_NUM_COUNT,
};

enum sys_scores_steam_state {
	SYS_SCORES_STATE_NONE,

	SYS_SCORES_STATE_FINDING,
	SYS_SCORES_STATE_LOADING,

	SYS_SCORES_STATE_NUM_COUNT,
};

struct sys_steam_scores_req_get {
	struct alloc alloc;
};

struct sys_steam_scores_req_add {
	u32 value;
};

struct sys_steam_scores_req {
	u32 id;
	str8 board_id;
	b32 cancelled;
	enum sys_scores_req_type type;
	enum sys_scores_steam_state state;
	sys_steam_api_call api_call;
	sys_steam_board_handle board_handle;
	sys_scores_req_callback callback;
	void *userdata;

	union {
		struct sys_steam_scores_req_get get;
		struct sys_steam_scores_req_add add;
	};
};

struct sys_scores_state {
	u32 next_id;
	b16 busy;
	u8 start;
	u8 end;
	struct sys_steam_scores_req reqs[SYS_SCORES_REQS_QUEUE_CAP];
};

static struct sys_scores_state SCORES_QUERIES_STATE;
static struct sys_scores_state SCORES_MUTATIONS_STATE;
static struct ht_entry_u64 SCORES_LB_CACHE_ENTRIES[1 << SYS_SCORES_LB_CACHE_EXP];
static struct ht_u64 SCORES_LB_CACHE = {
	.exp = SYS_SCORES_LB_CACHE_EXP,
	.ht  = SCORES_LB_CACHE_ENTRIES,
};

static const str8 SYS_SCORES_ERR_UNAVAILABLE = str8_lit_comp("steam unavailable");
static const str8 SYS_SCORES_ERR_NOT_FOUND   = str8_lit_comp("leaderboard not found");
static const str8 SYS_SCORES_ERR_CALL_FAIL   = str8_lit_comp("steam call failed");
static const str8 SYS_SCORES_ERR_UPLOAD      = str8_lit_comp("score upload failed");
static const str8 SYS_SCORES_PLAYER_UNKNOWN  = str8_lit_comp("?");

static i32 sys_scores_steam_score_encode(u32 value);
static u32 sys_scores_steam_score_decode(i32 value);
static b32 sys_scores_board_id_to_name(str8 board_id, char *out, usize out_cap);
static b32 sys_scores_start_req(struct sys_scores_state *state, struct sys_steam_scores_req *req);
static void sys_scores_req_fail(struct sys_scores_state *state, enum sys_scores_res_type type, str8 error_message);
static enum sys_scores_res_type sys_scores_res_by_req_type(enum sys_scores_req_type type);
static sys_steam_board_handle sys_scores_board_id_cache_get(str8 name);

static void sys_scores_req_finish(struct sys_scores_state *state, struct sys_scores_res res);
static str8 sys_scores_steam_user_name_get(ISteamFriends *users, sys_steam_id steam_id);

static void sys_scores_handle_board_found(struct sys_scores_state *state, struct sys_steam_scores_req *req, struct sys_steam_scoreboard_find *data);
static void sys_scores_handle_get(struct sys_scores_state *state, struct sys_steam_scores_req *req, struct sys_steam_scoreboard_get *data);
static void sys_scores_handle_add(struct sys_scores_state *state, struct sys_steam_scores_req *req, struct sys_steam_scoreboard_add *data);

static void sys_scores_start_next(struct sys_scores_state *state);
static void sys_scores_poll_state(struct sys_scores_state *state);
static int sys_scores_queue_clear(struct sys_scores_state *state);
static int sys_scores_queue_push(
	struct sys_scores_state *state,
	enum sys_scores_req_type type,
	str8 board_id,
	u32 value,
	sys_scores_req_callback callback,
	void *userdata,
	struct alloc alloc);

void
sys_steam_scores_tick(void)
{
	sys_scores_poll_state(&SCORES_QUERIES_STATE);
	sys_scores_poll_state(&SCORES_MUTATIONS_STATE);
}

int
sys_scores_queries_clear_queue(void)
{
	log_info("sys-scores", "Clear scores queries queue, start: %d, end: %d", (int)SCORES_QUERIES_STATE.start, (int)SCORES_QUERIES_STATE.end);
	return sys_scores_queue_clear(&SCORES_QUERIES_STATE);
}

int
sys_scores_mutations_clear_queue(void)
{
	log_info("sys-scores", "Clear scores mutations queue, start: %d, end: %d", (int)SCORES_MUTATIONS_STATE.start, (int)SCORES_MUTATIONS_STATE.end);
	return sys_scores_queue_clear(&SCORES_MUTATIONS_STATE);
}

int
sys_score_add(str8 board_id, u32 value, sys_scores_req_callback callback, void *userdata)
{
	return sys_scores_queue_push(
		&SCORES_MUTATIONS_STATE,
		SYS_SCORES_REQ_ADD,
		board_id,
		value,
		callback,
		userdata,
		(struct alloc){0});
}

int
sys_scores_get(str8 board_id, sys_scores_req_callback callback, void *userdata, struct alloc alloc)
{
	return sys_scores_queue_push(
		&SCORES_QUERIES_STATE,
		SYS_SCORES_REQ_GET,
		board_id,
		0,
		callback,
		userdata,
		alloc);
}

int
sys_scores_personal_best_get(
	str8 board_id,
	sys_scores_req_callback callback,
	void *userdata)
{
	return sys_scores_queue_push(
		&SCORES_QUERIES_STATE,
		SYS_SCORES_REQ_PERSONAL_BEST,
		board_id,
		0,
		callback,
		userdata,
		(struct alloc){0});
}

static int
sys_scores_queue_push(
	struct sys_scores_state *state,
	enum sys_scores_req_type type,
	str8 board_id,
	u32 value,
	sys_scores_req_callback callback,
	void *userdata,
	struct alloc alloc)
{
	int res = -1;

	dbg_check_warn(sys_steam_ok(), "sys-scores", "steam not ok");
	u8 next = (u8)((state->end + 1) % ARRLEN(state->reqs));
	dbg_check(next != state->start, "sys-scores", "Score queue full");
	dbg_assert(state->start < ARRLEN(state->reqs));
	dbg_assert(state->end < ARRLEN(state->reqs));

	struct sys_steam_scores_req *req = state->reqs + state->end;
	*req                             = (struct sys_steam_scores_req){
		.id        = state->next_id++,
		.board_id  = board_id,
		.cancelled = false,
		.type      = type,
		.callback  = callback,
		.userdata  = userdata,
	};

	switch(type) {
	case SYS_SCORES_REQ_GET: {
		req->get.alloc = alloc;
	} break;

	case SYS_SCORES_REQ_ADD: {
		req->add.value = value;
		log_info("sys-scores", "Queue add score for %.*s: %" PRIu32, str8_spread(board_id), value);
	} break;

	case SYS_SCORES_REQ_PERSONAL_BEST: {
	} break;

	default: {
		dbg_sentinel("sys-scores");
	} break;
	}

	state->end = next;
	res        = 0;
	if(!state->busy) { sys_scores_start_next(state); }

	return res;

error:
	if(callback) {
		struct sys_scores_res score_res = {
			.type          = sys_scores_res_by_req_type(type),
			.error_message = SYS_SCORES_ERR_UNAVAILABLE,
		};
		callback(0, score_res, userdata);
	}
	return -1;
}

static void
sys_scores_poll_state(struct sys_scores_state *state)
{
	if(!state->busy || state->start == state->end) { return; }

	struct sys_steam_scores_req *req = state->reqs + state->start;
	if(req->api_call == 0) { return; }

	ISteamUtils *utils = SteamAPI_SteamUtils_v011();

	if(!utils) { goto error; }

	bool failed = false;
	if(!SteamAPI_ISteamUtils_IsAPICallCompleted(utils, req->api_call, &failed)) { return; }

	sys_steam_api_call call                 = req->api_call;
	enum sys_scores_steam_state steam_state = req->state;
	req->api_call                           = 0;

	if(failed) { goto error; }

	if(steam_state == SYS_SCORES_STATE_FINDING) {
		struct sys_steam_scoreboard_find data = {0};
		b32 res                               = SteamAPI_ISteamUtils_GetAPICallResult(
			utils,
			call,
			&data,
			(i32)sizeof(data),
			SYS_STEAM_SCOREBOARD_FIND_DATA_SIZE,
			&failed);

		if(!res || failed) { goto error; }

		sys_scores_handle_board_found(state, req, &data);
	} else if(steam_state == SYS_SCORES_STATE_LOADING &&
		(req->type == SYS_SCORES_REQ_GET || req->type == SYS_SCORES_REQ_PERSONAL_BEST)) {
		struct sys_steam_scoreboard_get data = {0};
		b32 res                              = SteamAPI_ISteamUtils_GetAPICallResult(
			utils,
			call,
			&data,
			(i32)sizeof(data),
			SYS_STEAM_SCOREBOARD_GET_DATA_SIZE,
			&failed);

		if(!res || failed) { goto error; }

		sys_scores_handle_get(state, req, &data);
	} else if(steam_state == SYS_SCORES_STATE_LOADING && req->type == SYS_SCORES_REQ_ADD) {
		struct sys_steam_scoreboard_add data = {0};
		b32 res                              = SteamAPI_ISteamUtils_GetAPICallResult(
			utils,
			call,
			&data,
			(i32)sizeof(data),
			SYS_STEAM_SCOREBOARD_ADD_DATA_SIZE,
			&failed);

		if(!res || failed) { goto error; }

		sys_scores_handle_add(state, req, &data);
	} else {
		goto error;
	}
	return;

error:
	sys_scores_req_fail(state, sys_scores_res_by_req_type(req->type), SYS_SCORES_ERR_CALL_FAIL);
}

static void
sys_scores_start_next(struct sys_scores_state *state)
{
	dbg_assert(state->start < ARRLEN(state->reqs));
	dbg_assert(state->end < ARRLEN(state->reqs));

	if(state->start == state->end) {
		state->busy = false;
		return;
	}

	struct sys_steam_scores_req *req = state->reqs + state->start;
	str8 error_message               = {0};
	state->busy                      = true;
	req->api_call                    = 0;
	req->state                       = SYS_SCORES_STATE_NONE;

	if(!sys_steam_ok()) {
		error_message = SYS_SCORES_ERR_UNAVAILABLE;
		goto error;
	}

	ISteamUserStats *user_stats = SteamAPI_SteamUserStats_v013();

	if(!user_stats) {
		error_message = SYS_SCORES_ERR_CALL_FAIL;
		goto error;
	}

	str8 board_id                 = req->board_id;
	sys_steam_board_handle cached = sys_scores_board_id_cache_get(board_id);

	if(cached != 0) {
		req->board_handle = cached;
		sys_scores_start_req(state, req);
		return;
	}

	char board_name[SYS_SCORES_LB_NAME_MAX];
	if(!sys_scores_board_id_to_name(board_id, board_name, sizeof(board_name))) {
		error_message = SYS_SCORES_ERR_NOT_FOUND;
		goto error;
	}

	sys_steam_api_call call = SteamAPI_ISteamUserStats_FindLeaderboard(user_stats, board_name);
	if(call == 0) {
		error_message = SYS_SCORES_ERR_CALL_FAIL;
		goto error;
	}

	req->state    = SYS_SCORES_STATE_FINDING;
	req->api_call = call;

	return;

error:
	sys_scores_req_fail(state, sys_scores_res_by_req_type(req->type), error_message);
}

static b32
sys_scores_start_req(
	struct sys_scores_state *state,
	struct sys_steam_scores_req *req)
{
	ISteamUserStats *user_stats = SteamAPI_SteamUserStats_v013();

	if(!user_stats || req->board_handle == 0) { goto error; }

	if(req->type == SYS_SCORES_REQ_ADD) {
		i32 score = sys_scores_steam_score_encode(req->add.value);
		log_info("sys-scores", "Adding score for %.*s: %" PRIu32, str8_spread(req->board_id), req->add.value);

		sys_steam_api_call call = SteamAPI_ISteamUserStats_UploadLeaderboardScore(
			user_stats,
			req->board_handle,
			SYS_STEAM_SCORES_UPLOAD_KEEP_BEST,
			score,
			NULL,
			0);

		if(call == 0) { goto error; }

		req->state    = SYS_SCORES_STATE_LOADING;
		req->api_call = call;
	} else if(req->type == SYS_SCORES_REQ_PERSONAL_BEST) {
		ISteamUser *user = SteamAPI_SteamUser_v023();
		sys_steam_id users[1];
		users[0] = user ? SteamAPI_ISteamUser_GetSteamID(user) : 0;
		if(users[0] == 0) { goto error; }

		sys_steam_api_call call = SteamAPI_ISteamUserStats_DownloadLeaderboardEntriesForUsers(
			user_stats,
			req->board_handle,
			users,
			1);

		if(call == 0) { goto error; }

		req->state    = SYS_SCORES_STATE_LOADING;
		req->api_call = call;
	} else if(req->type == SYS_SCORES_REQ_GET) {
		sys_steam_api_call call = SteamAPI_ISteamUserStats_DownloadLeaderboardEntries(
			user_stats,
			req->board_handle,
			SYS_STEAM_SCORES_DATA_GLOBAL,
			1,
			SYS_STEAM_SCORES_TOP_ENTRY_COUNT);

		if(call == 0) { goto error; }

		req->state    = SYS_SCORES_STATE_LOADING;
		req->api_call = call;
	} else {
		goto error;
	}

	return true;

error:
	sys_scores_req_fail(state, sys_scores_res_by_req_type(req->type), SYS_SCORES_ERR_CALL_FAIL);
	return false;
}

static void
sys_scores_handle_get(struct sys_scores_state *state, struct sys_steam_scores_req *req, struct sys_steam_scoreboard_get *data)
{
	ISteamUserStats *user_stats = SteamAPI_SteamUserStats_v013();
	ISteamFriends *users        = SteamAPI_SteamFriends_v018();
	ISteamUser *user            = SteamAPI_SteamUser_v023();
	sys_steam_id local_id       = user ? SteamAPI_ISteamUser_GetSteamID(user) : 0;

	struct sys_scores_res res = {.type = sys_scores_res_by_req_type(req->type)};

	if(!user_stats) { goto error; }

	if(req->type == SYS_SCORES_REQ_PERSONAL_BEST) {
		if(data->count <= 0) {
			log_info("sys-scores", "No personal best for board %.*s", str8_spread(req->board_id));
			goto cleanup;
		}

		struct sys_steam_scoreboard_entry entry = {0};
		if(!SteamAPI_ISteamUserStats_GetDownloadedLeaderboardEntry(
			   user_stats,
			   data->entries,
			   0,
			   &entry,
			   NULL,
			   0)) {
			goto cleanup;
		}

		res.personal_best.score = (struct sys_score){
			.rank   = (u32)MAX(entry.global_rank, 0),
			.value  = sys_scores_steam_score_decode(entry.score),
			.player = sys_scores_steam_user_name_get(users, entry.steam_id),
		};

		log_info(
			"sys-scores",
			"Personal best, board:%.*s rank:%" PRIu32 " score:%" PRIu32 " player:%.*s",
			str8_spread(req->board_id),
			res.personal_best.score.rank,
			res.personal_best.score.value,
			str8_spread(res.personal_best.score.player));

		goto cleanup;
	}

	dbg_assert(req->get.alloc.allocf != NULL);
	i32 count = clamp_i32(data->count, 0, SYS_STEAM_SCORES_TOP_ENTRY_COUNT);
	res       = (struct sys_scores_res){
		.type = SYS_SCORE_RES_SCORES_GET,
		.get  = {
			.board_id     = req->board_id,
			.last_updated = sys_epoch_2000(NULL),
		},
	};

	struct sys_score_arr *entries = &res.get.entries;
	if(count > 0) {
		entries->items = alloc_arr(req->get.alloc, entries->items, count);
	}

	if(count > 0 && entries->items == NULL) { goto error; }

	entries->cap = (usize)count;
	entries->len = 0;

	for(i32 i = 0; i < count; ++i) {
		struct sys_steam_scoreboard_entry entry = {0};
		if(!SteamAPI_ISteamUserStats_GetDownloadedLeaderboardEntry(
			   user_stats,
			   data->entries,
			   i,
			   &entry,
			   NULL,
			   0)) {
			continue;
		}

		if(entry.steam_id == local_id) {
			res.get.player_included = true;
		}

		entries->items[entries->len++] = (struct sys_score){
			.rank   = (u32)MAX(entry.global_rank, 0),
			.value  = sys_scores_steam_score_decode(entry.score),
			.player = sys_scores_steam_user_name_get(users, entry.steam_id),
		};
	}

	log_info(
		"sys-scores",
		"Scores, board:%.*s count:%d limit:%d playerIncluded:%d lastUpdated:%" PRIu32,
		str8_spread(req->board_id),
		(int)entries->len,
		(int)SYS_STEAM_SCORES_TOP_ENTRY_COUNT,
		(int)res.get.player_included,
		res.get.last_updated);
	for(usize i = 0; i < entries->len; ++i) {
		log_info("sys-scores", "%" PRIu32 ". %.*s: %" PRIu32, entries->items[i].rank, str8_spread(entries->items[i].player), entries->items[i].value);
	}

cleanup:
	sys_scores_req_finish(state, res);
	return;

error:
	sys_scores_req_fail(state, sys_scores_res_by_req_type(req->type), SYS_SCORES_ERR_CALL_FAIL);
}

static void
sys_scores_handle_add(struct sys_scores_state *state, struct sys_steam_scores_req *req, struct sys_steam_scoreboard_add *data)
{
	if(!data->success) {
		sys_scores_req_fail(state, SYS_SCORE_RES_SCORES_ADD, SYS_SCORES_ERR_UPLOAD);
	} else {
		ISteamFriends *friends    = SteamAPI_SteamFriends_v018();
		ISteamUser *user          = SteamAPI_SteamUser_v023();
		sys_steam_id local_id     = user ? SteamAPI_ISteamUser_GetSteamID(user) : 0;
		struct sys_scores_res res = {
			.type = SYS_SCORE_RES_SCORES_ADD,
			.add  = {
				.score = {
					.rank   = (u32)MAX(data->rank_new, 0),
					.value  = sys_scores_steam_score_decode(data->score),
					.player = sys_scores_steam_user_name_get(friends, local_id),
				},
			},
		};

		log_info(
			"sys-scores",
			"Submitted score for board %.*s: %" PRIu32 ". %.*s %" PRIu32,
			str8_spread(req->board_id),
			res.add.score.rank,
			str8_spread(res.add.score.player),
			res.add.score.value);

		sys_scores_req_finish(state, res);
	}
}

static sys_steam_board_handle
sys_scores_board_id_cache_get(str8 board_id)
{
	u64 key = hash_fnv1a_str8(board_id);
	return key != 0 ? ht_get_u64(&SCORES_LB_CACHE, key) : 0;
}

static void
sys_scores_board_id_cache_put(str8 board_id, sys_steam_board_handle handle)
{
	if(handle == 0) { goto error; }

	u64 key = hash_fnv1a_str8(board_id);

	if(key == 0) { goto error; } // Zero is reserved for empty hash-table slots.
	if(ht_get_u64(&SCORES_LB_CACHE, key) != 0) { goto error; }

	dbg_check_warn(SCORES_LB_CACHE.len < SYS_SCORES_LB_CACHE_CAP, "sys-scores", "leaderboard cache full");
	ht_set_u64(&SCORES_LB_CACHE, key, handle);

error:;
}

// Bias the full unsigned range into Steam's signed range, preserving score order.
static i32
sys_scores_steam_score_encode(u32 value)
{
	return (i32)((i64)value - SYS_SCORES_STEAM_SCORE_BIAS);
}

static u32
sys_scores_steam_score_decode(i32 value)
{
	return (u32)((i64)value + SYS_SCORES_STEAM_SCORE_BIAS);
}

static str8
sys_scores_steam_user_name_get(ISteamFriends *users, sys_steam_id steam_id)
{
	str8 name = SYS_SCORES_PLAYER_UNKNOWN;
	if(users) {
		const char *persona = SteamAPI_ISteamFriends_GetFriendPersonaName(users, steam_id);
		if(persona && persona[0] != '\0') {
			name = str8_cstr((char *)persona);
		}
	}
	return name;
}

static void
sys_scores_req_finish(struct sys_scores_state *state, struct sys_scores_res res)
{
	struct sys_steam_scores_req *req = state->reqs + state->start;
	if(req->callback && !req->cancelled) {
		req->callback(req->id, res, req->userdata);
	}
	req->state    = SYS_SCORES_STATE_NONE;
	req->api_call = 0;
	state->start  = (u8)((state->start + 1) % ARRLEN(state->reqs));
	state->busy   = false;
	sys_scores_start_next(state);
}

static void
sys_scores_req_fail(struct sys_scores_state *state, enum sys_scores_res_type type, str8 error_message)
{
	struct sys_scores_res res = {
		.type          = type,
		.error_message = error_message,
	};
	log_error("sys-scores", "%.*s", str8_spread(error_message));
	sys_scores_req_finish(state, res);
}

static enum sys_scores_res_type
sys_scores_res_by_req_type(enum sys_scores_req_type type)
{
	enum sys_scores_res_type res_type = SYS_SCORE_RES_SCORES_NONE;
	switch(type) {
	case SYS_SCORES_REQ_GET: {
		res_type = SYS_SCORE_RES_SCORES_GET;
	} break;
	case SYS_SCORES_REQ_ADD: {
		res_type = SYS_SCORE_RES_SCORES_ADD;
	} break;
	case SYS_SCORES_REQ_PERSONAL_BEST: {
		res_type = SYS_SCORE_RES_SCORES_PERSONAL_BEST_GET;
	} break;
	default: {
	} break;
	}
	return res_type;
}

static b32
sys_scores_board_id_to_name(str8 board_id, char *out, usize out_cap)
{
	b32 ok = false;
	if(board_id.size > 0 && board_id.size + 1 <= out_cap && board_id.str != NULL) {
		str8 dst = {.str = (u8 *)out};
		str8_cpy(&board_id, &dst);
		ok = true;
	}
	return ok;
}

static void
sys_scores_handle_board_found(
	struct sys_scores_state *state,
	struct sys_steam_scores_req *req,
	struct sys_steam_scoreboard_find *data)
{
	if(!data->found || data->board_handle == 0) {
		sys_scores_req_fail(state, sys_scores_res_by_req_type(req->type), SYS_SCORES_ERR_NOT_FOUND);
	} else {
		req->board_handle = data->board_handle;
		sys_scores_board_id_cache_put(req->board_id, req->board_handle);
		sys_scores_start_req(state, req);
	}
}

static int
sys_scores_queue_clear(struct sys_scores_state *state)
{
	if(!state->busy) {
		state->start = 0;
		state->end   = 0;
	} else {
		struct sys_steam_scores_req *req = state->reqs + state->start;
		req->cancelled                   = true;
		state->end                       = (u8)((state->start + 1) % ARRLEN(state->reqs));
	}
	return 0;
}
