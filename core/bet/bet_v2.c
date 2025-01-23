#include "bet_v2.h"
#include "bet/bet.h"
#include "rndm.h"
#include "sys-log.h"
#include "sys-types.h"
#include "sys-utils.h"

enum bet_res bet_v2_tick_action(struct bet *bet, struct bet_ctx *ctx, usize node_index, void *userdata);
void bet_v2_tick_deco(struct bet *bet, struct bet_ctx *ctx, usize node_index, enum bet_res *res, void *userdata);
void bet_v2_tick_comp(struct bet *bet, struct bet_ctx *ctx, usize node_index, enum bet_res *res, i32 i, void *userdata);

void bet_v2_on_comp_init(struct bet *bet, struct bet_ctx *ctx, u8 node_index, void *userdata);
void bet_v2_on_deco_init(struct bet *bet, struct bet_ctx *ctx, u8 node_index, void *userdata);

static inline void bet_v2_set_child(struct bet *bet, struct bet_ctx *ctx, u8 node_index, i32 i);
static inline void bet_v2_finish_comp(struct bet *bet, struct bet_ctx *ctx, u8 node_index);

enum bet_res
bet_v2_tick(struct bet *bet, struct bet_ctx *ctx, void *userdata)
{
	enum bet_res res = BET_RES_NONE;

	// If the ctx is not initialized set the node_index to 1 as 0 is NULL

	{
		u8 curr_index                 = ctx->current;
		struct bet_node *curr_node    = bet_get_node(bet, curr_index);
		struct bet_node_ctx *curr_ctx = ctx->bet_node_ctx + curr_index;
		if(ctx->debug) {
			sys_printf("\ntick: [%d] %s -> %d", (int)curr_index, curr_node->name, (int)curr_ctx->i);
		}
	}

	for(;;) {
		u8 curr_index                 = ctx->current;
		struct bet_node *curr_node    = bet_get_node(bet, curr_index);
		struct bet_node_ctx *curr_ctx = ctx->bet_node_ctx + curr_index;
		i32 i                         = curr_ctx->i++;

		if(ctx->debug) {
			sys_printf("loop: i:%2d res:%-10s [%d] %s ", (int)i, BET_RES_STR[res], (int)curr_index, curr_node->name);
		}

		// On Node enter
		if(i == -1) {
			switch(curr_node->type) {
			case BET_NODE_COMP: {
				// If rndm set i to random
				bet_v2_on_comp_init(bet, ctx, curr_index, userdata);
				continue;
			} break;
			case BET_NODE_DECO: {
				// Reset children run_count
				bet_v2_on_deco_init(bet, ctx, curr_index, userdata);
				continue;
			} break;
			case BET_NODE_ACTION: {
				if(ctx->action_init != NULL) {
					ctx->action_init(bet, ctx, curr_node, userdata);
				}
				continue;
			} break;
			default: {
				BAD_PATH;
			} break;
			}
		}

		// Process the last node of parent
		// End the comp and jump to the parent
		// modify the res based on deco
		// run the action
		if(i == curr_node->children_count) {
			// Finished the tree (reset ctx?)
			if(!curr_node->parent) {
				if(ctx->debug) {
					sys_printf("root: [%d] %s -> %s", (int)curr_index, curr_node->name, BET_RES_STR[res]);
				}
				bet_v2_ctx_init(ctx);
				return res;
			}

			switch(curr_node->type) {
			case BET_NODE_COMP: {
			} break;
			case BET_NODE_DECO: {
				// Check if should be running
				// Modify res
				bet_v2_tick_deco(bet, ctx, curr_index, &res, userdata);
				if(ctx->debug) {
					sys_printf("deco: [%d] %s -> %s", (int)curr_index, curr_node->name, BET_RES_STR[res]);
				}
				if(res == BET_RES_RUNNING) {
					curr_ctx->i = 0;
					return res;
				}
			} break;
			case BET_NODE_ACTION: {
				res = bet_v2_tick_action(bet, ctx, curr_index, userdata);
				if(ctx->debug) {
					sys_printf("action: [%d] %s -> %s", (int)curr_index, curr_node->name, BET_RES_STR[res]);
				}
				if(res == BET_RES_RUNNING) {
					curr_ctx->i = 0;
					return res;
				}
			} break;
			default: {
				BAD_PATH;
			} break;
			}

			// Go back to parent
			ctx->current = curr_node->parent;
			continue;
		}

		switch(curr_node->type) {
		case BET_NODE_COMP: {
			bet_v2_tick_comp(bet, ctx, curr_index, &res, i, userdata);
		} break;
		case BET_NODE_DECO: {
			// Push on next child node
			bet_v2_set_child(bet, ctx, curr_index, i);
		} break;
		case BET_NODE_ACTION: {
			// Push on next child node
			bet_v2_set_child(bet, ctx, curr_index, i);
		} break;
		default: {
			BAD_PATH;
		} break;
		}
	}

	BAD_PATH;
	return res;
}

void
bet_v2_ctx_init(struct bet_ctx *ctx)
{
	ctx->current           = 1;
	ctx->bet_node_ctx[1].i = -1;
}

enum bet_res
bet_v2_tick_action(
	struct bet *bet,
	struct bet_ctx *ctx,
	usize node_index,
	void *userdata)
{
	struct bet_node *node = bet_get_node(bet, node_index);
	assert(node->type == BET_NODE_ACTION);
	enum bet_res res = BET_RES_NONE;
	if(ctx->action_do != NULL) {
		res = ctx->action_do(bet, ctx, node, userdata);
		assert(res != BET_RES_NONE);
	}

	return res;
}

void
bet_v2_tick_deco(
	struct bet *bet,
	struct bet_ctx *ctx,
	usize node_index,
	enum bet_res *res,
	void *userdata)
{
	struct bet_node *node         = bet_get_node(bet, node_index);
	struct bet_node_ctx *node_ctx = ctx->bet_node_ctx + node_index;
	assert(node->type == BET_NODE_DECO);
	assert(node->children_count == 1);
	enum bet_deco_type type = node->sub_type;
	node_ctx->run_count++;

	switch(type) {
	case BET_DECO_INVERT: {
		if(*res == BET_RES_SUCCESS) {
			*res = BET_RES_FAILURE;
		} else if(*res == BET_RES_FAILURE) {
			*res = BET_RES_SUCCESS;
		} else {
			BAD_PATH;
		}
	} break;
	case BET_DECO_FAILURE: {
		*res = BET_RES_FAILURE;
	} break;
	case BET_DECO_SUCCESS: {
		*res = BET_RES_SUCCESS;
	} break;
	case BET_DECO_ONE_SHOT: {
		// // TODO: IDK
		// res = BET_RES_SUCCESS;
		// TODO: When comp parallel is setup
		NOT_IMPLEMENTED;
	} break;
	case BET_DECO_REPEAT_X_TIMES: {
		struct bet_prop prop = node->props[0];
		assert(prop.type == BET_PROP_F32);
		i32 value = prop.f32;
		// If the run count is less than the prop trap in running
		if(*res == BET_RES_FAILURE) {
			*res = BET_RES_FAILURE;
		} else {
			if(node_ctx->run_count < value) {
				*res = BET_RES_RUNNING;
			} else {
				*res = BET_RES_SUCCESS;
			}
		}
	} break;
	case BET_DECO_REPEAT_UNTIL_SUCCESS: {
		if(*res != BET_RES_SUCCESS) {
			*res = BET_RES_RUNNING;
		}
	} break;
	case BET_DECO_REPEAT_UNTIL_FAILURE: {
		if(*res != BET_RES_FAILURE) {
			*res = BET_RES_RUNNING;
		}
	} break;
	default: {
		NOT_IMPLEMENTED;
	} break;
	}
}

void
bet_v2_tick_comp(
	struct bet *bet,
	struct bet_ctx *ctx,
	usize node_index,
	enum bet_res *res,
	i32 i,
	void *userdata)
{
	struct bet_node *node         = bet_get_node(bet, node_index);
	struct bet_node_ctx *node_ctx = ctx->bet_node_ctx + node_index;
	assert(node->type == BET_NODE_COMP);
	enum bet_comp_type type = node->sub_type;

	switch(type) {
	case BET_COMP_SEQUENCE: {
		if(*res == BET_RES_SUCCESS || *res == BET_RES_NONE) {
			bet_v2_set_child(bet, ctx, node_index, i);
		} else {
			*res = BET_RES_FAILURE;
			bet_v2_finish_comp(bet, ctx, node_index);
		}
	} break;
	case BET_COMP_SELECTOR: {
		if(*res == BET_RES_SUCCESS) {
			bet_v2_finish_comp(bet, ctx, node_index);
		} else {
			bet_v2_set_child(bet, ctx, node_index, i);
			*res = BET_RES_NONE;
		}
	} break;
		// TODO: Parallel comp
	case BET_COMP_RND: {
		bet_v2_finish_comp(bet, ctx, node_index);
	} break;
	case BET_COMP_RND_WEIGHTED: {
		bet_v2_finish_comp(bet, ctx, node_index);
	} break;
	default: {
	} break;
	}
}

void
bet_v2_on_deco_init(struct bet *bet, struct bet_ctx *ctx, u8 node_index, void *userdata)
{
	struct bet_node *node         = bet_get_node(bet, node_index);
	struct bet_node_ctx *node_ctx = ctx->bet_node_ctx + node_index;
	for(usize i = 0; i < node->children_count; ++i) {
		u8 child_index                 = node->children[i];
		struct bet_node *child         = bet_get_node(bet, child_index);
		struct bet_node_ctx *child_ctx = ctx->bet_node_ctx + child_index;
		child_ctx->run_count           = 0;
	}
}

void
bet_v2_on_comp_init(struct bet *bet, struct bet_ctx *ctx, u8 node_index, void *userdata)
{
	struct bet_node *node         = bet_get_node(bet, node_index);
	struct bet_node_ctx *node_ctx = ctx->bet_node_ctx + node_index;
	for(usize i = 0; i < node->children_count; ++i) {
		u8 child_index                 = node->children[i];
		struct bet_node *child         = bet_get_node(bet, child_index);
		struct bet_node_ctx *child_ctx = ctx->bet_node_ctx + child_index;
		child_ctx->run_count           = 0;
	}

	enum bet_comp_type type = node->sub_type;

	switch(type) {
	case BET_COMP_SEQUENCE: {
	} break;
	case BET_COMP_SELECTOR: {
	} break;
	case BET_COMP_RND: {
		usize rnd                    = rndm_range_i32(0, node->children_count - 1);
		u8 child_index               = node->children[rnd];
		ctx->current                 = child_index;
		struct bet_node_ctx *new_ctx = ctx->bet_node_ctx + ctx->current;
		new_ctx->i                   = -1;
	} break;
	case BET_COMP_RND_WEIGHTED: {
		struct rndm_weighted_choice choices[MAX_BET_CHILDREN] = {0};
		struct bet_prop weights                               = node->props[0];
		assert(weights.type == BET_PROP_U8_ARR);

		for(usize i = 0; i < node->children_count; ++i) {
			choices[i].key   = i;
			choices[i].value = weights.u8_arr[i];
		}
		usize rnd                    = rndm_weighted_choice_i32(choices, node->children_count);
		usize child_index            = node->children[rnd];
		ctx->current                 = child_index;
		struct bet_node_ctx *new_ctx = ctx->bet_node_ctx + ctx->current;
		new_ctx->i                   = -1;
	} break;
	default: {
	} break;
	}
}

static inline void
bet_v2_set_child(struct bet *bet, struct bet_ctx *ctx, u8 node_index, i32 i)
{
	struct bet_node *node        = bet_get_node(bet, node_index);
	usize child_index            = node->children[i];
	ctx->current                 = child_index;
	struct bet_node_ctx *new_ctx = ctx->bet_node_ctx + ctx->current;
	new_ctx->i                   = -1;
}

static inline void
bet_v2_finish_comp(struct bet *bet, struct bet_ctx *ctx, u8 node_index)
{
	struct bet_node *node         = bet_get_node(bet, node_index);
	struct bet_node_ctx *node_ctx = ctx->bet_node_ctx + node_index;
	node_ctx->i                   = node->children_count;
}
