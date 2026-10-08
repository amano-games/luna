#pragma once

#include "base/mem.h"
#include "base/types.h"

enum sys_scores_scope {
	SYS_SCORES_SCOPE_GLOBAL = 0,
	SYS_SCORES_SCOPE_FRIENDS,
};

enum sys_scores_res_type {
	SYS_SCORE_RES_SCORES_NONE,

	SYS_SCORE_RES_SCORES_GET,
	SYS_SCORE_RES_SCORES_ADD,
	SYS_SCORE_RES_SCORES_PERSONAL_BEST_GET,

	SYS_SCORE_RES_SCORES_NUM_COUNT,
};

struct sys_score {
	u32 value;
	u32 rank;
	str8 player;
	b32 is_player;
};

struct sys_score_arr {
	usize len;
	usize cap;
	struct sys_score *items;
};

struct sys_scores_res_get {
	str8 board_id;
	u32 last_updated;
	b32 player_included;
	struct sys_score_arr entries;
};

struct sys_scores_res_add {
	struct sys_score score;
};

struct sys_scores_res_personal_best {
	struct sys_score score;
};

struct sys_scores_res {
	str8 error_message;
	enum sys_scores_res_type type;
	union {
		struct sys_scores_res_get get;
		struct sys_scores_res_add add;
		struct sys_scores_res_personal_best personal_best;
	};
};

struct sys_scores_err {
	i32 type;
	str8 msg;
};

typedef void (*sys_scores_req_callback)(u32 id, struct sys_scores_res res, void *userdata);

// @per_os_impl Scoreboards

int sys_scores_queries_clear_queue(void);
int sys_scores_mutations_clear_queue(void);
int sys_score_add(str8 board_id, u32 value, sys_scores_req_callback callback, void *userdata);
int sys_scores_get(str8 board_id, enum sys_scores_scope scope, sys_scores_req_callback callback, void *userdata, struct alloc alloc);
int sys_scores_personal_best_get(str8 board_id, sys_scores_req_callback callback, void *userdata);
