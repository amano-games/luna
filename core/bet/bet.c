#include "bet.h"
#include "mem.h"
#include "rndm.h"
#include "sys-io.h"
#include "sys-log.h"
#include "sys-types.h"
#include "sys-assert.h"

void
bet_init(struct bet *bet)
{
	mclr(bet, sizeof(struct bet));
	bet->count = 1;
}

i32
bet_push_node(struct bet *bet, struct bet_node node)
{
	assert(bet != NULL);
	bet->nodes[bet->count++] = node;
	return bet->count - 1;
}

struct bet_node *
bet_get_node(struct bet *bet, usize node_index)
{
	assert(bet != NULL);
	assert(node_index > 0 && node_index < bet->count);
	return bet->nodes + node_index;
}

bool32
bet_push_child(struct bet *bet, usize parent_index, usize child_index)
{
	assert(parent_index < child_index);
	struct bet_node *parent = bet_get_node(bet, parent_index);
	assert(parent != NULL);
	assert(parent->type == BET_NODE_COMP || parent->type == BET_NODE_DECO);
	assert(parent->children_count + 1 < MAX_BET_CHILDREN);

	parent->children[parent->children_count++] = child_index;

	return true;
}

bool32
bet_push_prop(struct bet *bet, usize node_index, struct bet_prop prop)
{
	struct bet_node *node = bet_get_node(bet, node_index);
	assert(node->prop_count + 1 <= MAX_BET_NODE_PROPS);
	node->props[node->prop_count++] = prop;
	return true;
}

enum bet_res
bet_tick_sequence(struct bet *bet, struct bet_ctx *ctx, struct bet_node *node, void *userdata)
{
	assert(node->type == BET_NODE_COMP);
	assert(node->sub_type == BET_COMP_SEQUENCE);
	enum bet_res res = BET_RES_NONE;

	for(usize i = 0; i < node->children_count; ++i) {
		struct bet_node *child = bet_get_node(bet, node->children[i]);
		res                    = bet_tick_node(bet, ctx, node->children[i], userdata);
		if(res != BET_RES_SUCCESS) {
			return res;
		}
	}
	return res;
}

enum bet_res
bet_tick_selector(struct bet *bet, struct bet_ctx *ctx, struct bet_node *node, void *userdata)
{
	assert(node->type == BET_NODE_COMP);
	assert(node->sub_type == BET_COMP_SELECTOR);
	enum bet_res res = BET_RES_NONE;

	for(usize i = 0; i < node->children_count; ++i) {
		struct bet_node *child = bet_get_node(bet, node->children[i]);
		res                    = bet_tick_node(bet, ctx, node->children[i], userdata);
		if(res != BET_RES_FAILURE) {
			return res;
		}
	}
	return res;
}

enum bet_res
bet_tick_rnd(struct bet *bet, struct bet_ctx *ctx, struct bet_node *node, void *userdata)
{
	assert(node->type == BET_NODE_COMP);
	assert(node->sub_type == BET_COMP_RND);
	enum bet_res res = BET_RES_NONE;

	usize child_index = rndm_range_i32(0, node->children_count);
	return bet_tick_node(bet, ctx, node->children[child_index], userdata);
}

enum bet_res
bet_tick_rnd_weighted(struct bet *bet, struct bet_ctx *ctx, struct bet_node *node, void *userdata)
{
	assert(node->type == BET_NODE_COMP);
	assert(node->sub_type == BET_COMP_RND_WEIGHTED);
	enum bet_res res = BET_RES_NONE;

	struct rndm_weighted_choice choices[MAX_BET_CHILDREN] = {0};
	struct bet_prop weights                               = node->props[0];
	assert(weights.type == BET_PROP_I32_ARR);

	for(usize i = 0; i < node->children_count; ++i) {
		choices[i].key   = i;
		choices[i].value = weights.i32_arr[i];
	}
	usize child_index = rndm_weighted_choice_i32(choices, node->children_count);
	return bet_tick_node(bet, ctx, node->children[child_index], userdata);
}

enum bet_res
bet_tick_action(struct bet *bet, struct bet_ctx *ctx, struct bet_node *node, void *userdata)
{
	assert(node->type == BET_NODE_ACTION);
	enum bet_res res = BET_RES_NONE;
	res              = ctx->action_do(bet, ctx, node, userdata);
	sys_printf("  res: %s", BET_RES_STR[res]);
	assert(res != BET_RES_NONE);
	node->run_count++;

	return res;
}

enum bet_res
bet_tick_deco(struct bet *bet, struct bet_ctx *ctx, struct bet_node *node, void *userdata)
{
	assert(node->type == BET_NODE_DECO);
	assert(node->children_count == 1);
	enum bet_deco_type type = node->sub_type;
	enum bet_res res        = BET_RES_NONE;

	res = bet_tick_node(bet, ctx, node->children[0], userdata);

	switch(type) {
	case BET_DECO_INVERT: {
		if(res == BET_RES_SUCCESS) {
			res = BET_RES_FAILURE;
		} else if(res == BET_RES_FAILURE) {
			res = BET_RES_SUCCESS;
		}
		sys_printf("  deco-invert: %s", BET_RES_STR[res]);
	} break;
	case BET_DECO_FAILURE: {
		res = BET_RES_FAILURE;
	} break;
	case BET_DECO_SUCCESS: {
		res = BET_RES_SUCCESS;
	} break;
	// TODO: Reset memory ?
	// case BET_DECO_ONE_SHOT: {
	// 	if(node->run_count > 1) { res = BET_RES_FAILURE; }
	// } break;
	// case BET_DECO_REPEAT_X_TIMES: {
	// 	struct bet_prop prop = node->props[0];
	// 	assert(prop.type == BET_PROP_I32);
	// 	if((i32)node->run_count > prop.i32) { res = BET_RES_FAILURE; }
	// } break;
	default: {
		NOT_IMPLEMENTED;
	} break;
	}
	return res;
}

enum bet_res
bet_tick_comp(struct bet *bet, struct bet_ctx *ctx, struct bet_node *node, void *userdata)
{
	assert(node->type == BET_NODE_COMP);
	enum bet_res res        = BET_RES_NONE;
	enum bet_comp_type type = node->sub_type;
	switch(type) {
	case BET_COMP_SEQUENCE: {
		sys_printf("->");
		return bet_tick_sequence(bet, ctx, node, userdata);
	} break;
	case BET_COMP_SELECTOR: {
		sys_printf("?");
		return bet_tick_selector(bet, ctx, node, userdata);
	} break;
	case BET_COMP_RND: {
		sys_printf("rnd");
		return bet_tick_rnd(bet, ctx, node, userdata);
	} break;
	case BET_COMP_RND_WEIGHTED: {
		sys_printf("rnd weighted");
		return bet_tick_rnd_weighted(bet, ctx, node, userdata);
	} break;
	default: {
		NOT_IMPLEMENTED;
	} break;
	}
	return res;
}

enum bet_res
bet_tick(struct bet *bet, struct bet_ctx *ctx, void *userdata)
{
	enum bet_res res = BET_RES_NONE;
	if(bet->count == 0) { return res; }

	return bet_tick_node(bet, ctx, 1, userdata);
}

enum bet_res
bet_tick_node(struct bet *bet, struct bet_ctx *ctx, usize node_index, void *userdata)
{
	enum bet_res res      = BET_RES_NONE;
	struct bet_node *node = bet_get_node(bet, node_index);
	switch(node->type) {
	case BET_NODE_COMP: {
		return bet_tick_comp(bet, ctx, node, userdata);
	} break;
	case BET_NODE_ACTION: {
		return bet_tick_action(bet, ctx, node, userdata);
	} break;
	case BET_NODE_DECO: {
		return bet_tick_deco(bet, ctx, node, userdata);
	} break;
	default: {
		NOT_IMPLEMENTED;
	} break;
	}
	return res;
}

str8
bet_node_serialize(struct bet *bet, usize node_index, struct alloc scratch)
{
#if 0
	struct bet_node *node = bet_get_node(bet, node_index);
	char *buffer          = scratch.allocf(scratch.ctx, sizeof(char) * 200);

	switch(node->type) {
	case BET_NODE_COMP: {
		stbsp_sprintf(buffer, "node type: %s(%d) - %s(%d) child count: %d", BET_NODE_TYPE_STR[node->type], (int)node->type, BET_COMP_TYPE_STR[node->sub_type], node->sub_type, (int)node->children_count);
	} break;
	case BET_NODE_DECO: {
		stbsp_sprintf(buffer, "node type: %s(%d) - %s(%d)", BET_NODE_TYPE_STR[node->type], (int)node->type, BET_DECO_TYPE_STR[node->sub_type], (int)node->sub_type);
	} break;
	case BET_NODE_ACTION: {
		stbsp_sprintf(buffer, "node type: %s - %d: %s", BET_NODE_TYPE_STR[node->type], (int)node->sub_type, node->note);
	} break;
	default: {
		NOT_IMPLEMENTED;
	} break;
	}
	str8 res = str8_cstr(buffer);
#endif
	str8 res = {0};
	return res;
}

struct bet
bet_load(str8 path)
{
	struct bet bet = {0};
	void *f        = sys_file_open_r(path);
	log_info("Bet", "Load bet %s", path.str);
	if(f == NULL) {
		log_error("Bet", "failed to open bet file: %s", path.str);
		return bet;
	}
	sys_file_r(f, &bet, sizeof(struct bet));
	sys_file_close(f);
	return bet;
}
