#pragma once

#include "mem.h"
#include "serialize/serialize.h"
#include "sys-types.h"

#define MAX_BET_NODES      40
#define MAX_BET_CHILDREN   10
#define MAX_BET_NODE_PROPS 3
#define MAX_BET_NOTE       64

enum bet_node_type {
	BET_NODE_NONE,
	BET_NODE_ACTION,
	BET_NODE_COMP,
	BET_NODE_DECO,
	BET_NODE_NUM_COUNT,
};

static char *BET_NODE_TYPE_STR[BET_NODE_NUM_COUNT] = {
	[BET_NODE_NONE]   = "none",
	[BET_NODE_ACTION] = "action",
	[BET_NODE_COMP]   = "composite",
	[BET_NODE_DECO]   = "decorator",
};

enum bet_res {
	BET_RES_NONE,
	BET_RES_SUCCESS,
	BET_RES_FAILURE,
	BET_RES_RUNNING,
	BET_RES_NUM_COUNT,
};

static char *BET_RES_STR[BET_RES_NUM_COUNT] = {
	[BET_RES_NONE]    = "none",
	[BET_RES_SUCCESS] = "success",
	[BET_RES_FAILURE] = "failure",
	[BET_RES_RUNNING] = "running",
};

enum bet_comp_type {
	BET_COMP_NONE,
	BET_COMP_SELECTOR,
	BET_COMP_SEQUENCE,
	BET_COMP_RND,
	BET_COMP_RND_WEIGHTED,

	BET_COMP_NUM_COUNT,
};

static char *BET_COMP_TYPE_STR[BET_COMP_NUM_COUNT] = {
	[BET_COMP_NONE]         = "none",
	[BET_COMP_SELECTOR]     = "selector",
	[BET_COMP_SEQUENCE]     = "sequence",
	[BET_COMP_RND]          = "rnd",
	[BET_COMP_RND_WEIGHTED] = "rnd weighted",
};

enum bet_deco_type {
	BET_DECO_NONE,
	BET_DECO_FAILURE,
	BET_DECO_INVERT,
	BET_DECO_SUCCESS,
	BET_DECO_ONE_SHOT,
	BET_DECO_REPEAT_X_TIMES,
	BET_DECO_REPEAT_RND_TIMES,
	BET_DECO_REPEAT_UNTIL_FAILURE,
	BET_DECO_REPEAT_UNTIL_SUCCESS,

	BET_DECO_NUM_COUNT,
};

static char *BET_DECO_TYPE_STR[BET_DECO_NUM_COUNT] = {
	[BET_DECO_NONE]                 = "none",
	[BET_DECO_FAILURE]              = "failure",
	[BET_DECO_INVERT]               = "invert",
	[BET_DECO_SUCCESS]              = "success",
	[BET_DECO_ONE_SHOT]             = "one shot",
	[BET_DECO_REPEAT_X_TIMES]       = "repeat x times",
	[BET_DECO_REPEAT_RND_TIMES]     = "repeat rnd times",
	[BET_DECO_REPEAT_UNTIL_FAILURE] = "repeat until failure",
	[BET_DECO_REPEAT_UNTIL_SUCCESS] = "repeat until success",
};

enum bet_prop_type {
	BET_PROP_NONE,
	BET_PROP_BOOL32,
	BET_PROP_I32,
	BET_PROP_F32,
	BET_PROP_U8_ARR,
	BET_PROP_STR,
};

struct bet_prop {
	enum bet_prop_type type;
	union {
		bool32 bool32;
		i32 i32;
		f32 f32;
		u8 u8_arr[4];
		u8 str[4];
	};
};

struct bet_node {
	enum bet_node_type type;
	i16 sub_type;
	u8 parent;
	u8 i;
	u8 children_count;
	u8 children[MAX_BET_CHILDREN];
	u8 prop_count;
	struct bet_prop props[MAX_BET_NODE_PROPS];
	char name[MAX_BET_NOTE];
};

struct bet {
	struct alloc alloc;
	struct bet_node *nodes;
};

struct bet_node_ctx {
	u8 run_count;
	u8 run_max;
	i32 i;
};

struct bet_ctx {
	u8 current;
	u8 initial;
	bool32 debug;

	struct bet_node_ctx bet_node_ctx[MAX_BET_NODES];
	void (*action_init)(struct bet *bet, struct bet_ctx *ctx, struct bet_node *node, void *userdata);
	enum bet_res (*action_do)(struct bet *bet, struct bet_ctx *ctx, struct bet_node *node, void *userdata);
};

void bet_init(struct bet *bet, struct alloc alloc);
struct bet_node *bet_get_node(struct bet *bet, usize node_index);
i32 bet_push_node(struct bet *bet, struct bet_node node);
bool32 bet_push_child(struct bet *bet, usize parent_index, usize child_index);
bool32 bet_push_prop(struct bet *bet, usize node_index, struct bet_prop prop);

enum bet_res bet_tick(struct bet *bet, struct bet_ctx *ctx, void *userdata);
enum bet_res bet_tick_node(struct bet *bet, struct bet_ctx *ctx, usize node_index, void *userdata);
str8 bet_node_serialize(struct bet *bet, usize node_index, struct alloc scratch);

// TODO: Move to assets
struct bet bet_load(str8 path, struct alloc alloc, struct alloc scratch);

void bet_node_write(struct ser_writer *w, struct bet_node n);
void bet_nodes_write(struct ser_writer *w, struct bet_node *nodes, usize count);
i32 bet_read(struct ser_reader *r, struct ser_value arr, struct bet *bet, usize max_count);

f32 bet_prop_f32_get(struct bet_prop prop, f32 fallback);
i32 bet_prop_i32_get(struct bet_prop prop, i32 fallback);
