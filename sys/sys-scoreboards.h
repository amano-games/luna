#pragma once

#include "mem.h"
#include "sys-types.h"

enum sys_scores_req_state {
	SYS_SCORE_REQ_STATE_NONE,

	SYS_SCORE_REQ_STATE_QUEUE,
	SYS_SCORE_REQ_STATE_LOADING,
	SYS_SCORE_REQ_STATE_IDLE,

	SYS_SCORE_REQ_STATE_NUM_COUNT,
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
};

struct sys_score_arr {
	usize len;
	usize cap;
	struct sys_score *items;
};

struct sys_scores_res_get {
	str8 board_id;
	u32 last_updated;
	bool32 player_included;
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

int sys_scores_clear_queue(void);
int sys_score_add(str8 board_id, u32 value, sys_scores_req_callback callback, void *userdata);
int sys_scores_get(str8 board_id, sys_scores_req_callback callback, void *userdata, struct alloc alloc);
int sys_scores_personal_best_get(str8 board_id, sys_scores_req_callback callback, void *userdata);
