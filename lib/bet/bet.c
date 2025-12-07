#include "bet.h"
#include "base/arr.h"
#include "base/mem.h"
#include "lib/rndm.h"
#include "base/types.h"
#include "base/dbg.h"

void bet_comp_init(struct bet *bet, struct bet_ctx *ctx, u8 node_index, void *userdata);
void bet_deco_init(struct bet *bet, struct bet_ctx *ctx, u8 node_index, void *userdata);

void bet_comp_tick(struct bet *bet, struct bet_ctx *ctx, usize node_index, enum bet_res *res, i32 i, void *userdata);
void bet_deco_tick(struct bet *bet, struct bet_ctx *ctx, usize node_index, enum bet_res *res, i32 i, void *userdata);
enum bet_res bet_action_tick(struct bet *bet, struct bet_ctx *ctx, usize node_index, void *userdata);

void bet_comp_end(struct bet *bet, struct bet_ctx *ctx, usize node_index, enum bet_res *res, void *userdata);
void bet_deco_end(struct bet *bet, struct bet_ctx *ctx, usize node_index, enum bet_res *res, void *userdata);

static inline void bet_set_index(struct bet *bet, struct bet_ctx *ctx, u8 node_index);
static inline void bet_set_child(struct bet *bet, struct bet_ctx *ctx, u8 node_index, i32 i);
static inline void bet_finish_comp(struct bet *bet, struct bet_ctx *ctx, u8 node_index);

static inline b32 bet_node_parent_is_parallel(struct bet *bet, struct bet_ctx *ctx, u8 node_index);

void
bet_ctx_init(struct bet_ctx *ctx)
{
	ctx->bet_node_ctx[ctx->current].i   = -1;
	ctx->bet_node_ctx[ctx->current].res = BET_RES_NONE;
	ctx->current                        = 1;
	ctx->bet_node_ctx[1].i              = -1;
}

enum bet_res
bet_tick(struct bet *bet, struct bet_ctx *ctx, void *userdata)
{
	enum bet_res res = BET_RES_NONE;

	// If the ctx is not initialized set the node_index to 1 as 0 is NULL

	{
		u8 curr_index                 = ctx->current;
		struct bet_node *curr_node    = bet_get_node(bet, curr_index);
		struct bet_node_ctx *curr_ctx = ctx->bet_node_ctx + curr_index;
		res                           = curr_ctx->res;
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
				bet_comp_init(bet, ctx, curr_index, userdata);
				res = BET_RES_NONE;
				continue;
			} break;
			case BET_NODE_DECO: {
				// Reset children run_count
				bet_deco_init(bet, ctx, curr_index, userdata);
				continue;
			} break;
			case BET_NODE_ACTION: {
				if(curr_ctx->res == BET_RES_RUNNING) { dbg_sentinel("bet"); }
				if(ctx->action_init != NULL) {
					ctx->action_init(bet, ctx, curr_node, userdata);
				}
				continue;
			} break;
			default: {
				dbg_sentinel("bet");
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
				bet_ctx_init(ctx);
				return res;
			}

			switch(curr_node->type) {
			case BET_NODE_COMP: {
				bet_comp_end(bet, ctx, curr_index, &res, userdata);

				if(res == BET_RES_RUNNING) {
					curr_ctx->i = 0;
					return res;
				}
			} break;
			case BET_NODE_DECO: {
				// Check if should be running
				// Modify res
				bet_deco_end(bet, ctx, curr_index, &res, userdata);
				if(ctx->debug) {
					sys_printf("deco: [%d] %s -> %s", (int)curr_index, curr_node->name, BET_RES_STR[res]);
				}
				if(res == BET_RES_RUNNING) {
					curr_ctx->i = 0;
					return res;
				}
			} break;
			case BET_NODE_ACTION: {
				enum bet_res action_res = bet_action_tick(bet, ctx, curr_index, userdata);
				if(ctx->debug) {
					sys_printf("action: [%d] %s -> %s", (int)curr_index, curr_node->name, BET_RES_STR[action_res]);
				}
				if(action_res == BET_RES_RUNNING) {
					curr_ctx->i = 0;
				}
				// TODO: This only works for actions that are inmediatly
				// children of the parallel node. If the node is child of a different comp node it will trap the tick here and not let the parent handle it.
				// I think we need to traverse thre tree back up until root but if the response was RUNNING don't continue the comps and let us get to the root.
				// And override the Running Index of the content based on deco or parallel nodes the running index
				//
				// Step 01: curr_index shouldn't modify the bet_ctx curr index
				// Step 02: don't inmediatly return when a node is running, let us evaluate the tree upwards until root
				// Step 03: handle comp or deco getting a running response, override the running_index depedenly
				// Only on root return save the running index to the context
				b32 parent_is_parallel = bet_node_parent_is_parallel(bet, ctx, curr_index);

				// If parallel parent and one of the nodes is
				// running we want the parallel parent to continue running
				// so it doesn't matter the new res
				if(parent_is_parallel) {
					if(res != BET_RES_RUNNING) {
						res = action_res;
					}
				} else {
					res = action_res;
					if(res == BET_RES_RUNNING) {
						return res;
					}
				}
			} break;
			default: {
				dbg_sentinel("bet");
			} break;
			}

			// Go back to parent
			ctx->current = curr_node->parent;
			continue;
		}

		switch(curr_node->type) {
		case BET_NODE_COMP: {
			bet_comp_tick(bet, ctx, curr_index, &res, i, userdata);
		} break;
		case BET_NODE_DECO: {
			bet_deco_tick(bet, ctx, curr_index, &res, i, userdata);
		} break;
		case BET_NODE_ACTION: {
			// Push on next child node
			bet_set_child(bet, ctx, curr_index, i);
		} break;
		default: {
			dbg_sentinel("bet");
		} break;
		}
	}

error:
	dbg_assert(0);
	return res;
}

void
bet_deco_init(
	struct bet *bet,
	struct bet_ctx *ctx,
	u8 node_index,
	void *userdata)
{
	struct bet_node *node         = bet_get_node(bet, node_index);
	struct bet_node_ctx *node_ctx = ctx->bet_node_ctx + node_index;
	for(usize i = 0; i < node->children_count; ++i) {
		u8 child_index                 = node->children[i];
		struct bet_node *child         = bet_get_node(bet, child_index);
		struct bet_node_ctx *child_ctx = ctx->bet_node_ctx + child_index;
		child_ctx->run_count           = 0;
	}
	enum bet_deco_type type = node->sub_type;

	switch(type) {
	case BET_DECO_REPEAT_X_TIMES: {
		i32 value         = bet_prop_f32_get(node->props[0], 0);
		node_ctx->run_max = value;
	} break;
	case BET_DECO_REPEAT_RND_TIMES: {
		i32 min           = bet_prop_f32_get(node->props[0], 0);
		i32 max           = bet_prop_f32_get(node->props[1], 0);
		i32 value         = rndm_range_i32(NULL, min, max);
		node_ctx->run_max = value;
	} break;
	default: {
	} break;
	}
}

void
bet_comp_init(struct bet *bet, struct bet_ctx *ctx, u8 node_index, void *userdata)
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
	case BET_COMP_PARALLEL: {
	} break;
	case BET_COMP_RND: {
		usize rnd = rndm_range_i32(NULL, 0, node->children_count - 1);
		bet_set_child(bet, ctx, node_index, rnd);
	} break;
	case BET_COMP_RND_WEIGHTED: {
		struct rndm_weighted_choice choices[MAX_BET_CHILDREN] = {0};
		struct bet_prop weights                               = node->props[0];
		dbg_assert(weights.type == BET_PROP_U8_ARR);

		for(usize i = 0; i < node->children_count; ++i) {
			choices[i].key   = i;
			choices[i].value = weights.u8_arr[i];
		}
		usize rnd = rndm_weighted_choice_i32(NULL, choices, node->children_count);
		bet_set_child(bet, ctx, node_index, rnd);
	} break;
	default: {
	} break;
	}
}

void
bet_comp_tick(
	struct bet *bet,
	struct bet_ctx *ctx,
	usize node_index,
	enum bet_res *res,
	i32 i,
	void *userdata)
{
	struct bet_node *node         = bet_get_node(bet, node_index);
	struct bet_node_ctx *node_ctx = ctx->bet_node_ctx + node_index;
	dbg_assert(node->type == BET_NODE_COMP);
	enum bet_comp_type type = node->sub_type;

	switch(type) {
	case BET_COMP_SEQUENCE: {
		if(*res == BET_RES_SUCCESS || *res == BET_RES_NONE) {
			bet_set_child(bet, ctx, node_index, i);
		} else {
			*res = BET_RES_FAILURE;
			bet_finish_comp(bet, ctx, node_index);
		}
	} break;
	case BET_COMP_SELECTOR: {
		if(*res == BET_RES_SUCCESS) {
			bet_finish_comp(bet, ctx, node_index);
		} else {
			bet_set_child(bet, ctx, node_index, i);
		}
	} break;
	case BET_COMP_PARALLEL: {
		if(*res == BET_RES_RUNNING) {
			// don't let children override running
		}
		bet_set_child(bet, ctx, node_index, i);
	} break;
	case BET_COMP_RND: {
		bet_finish_comp(bet, ctx, node_index);
		// *res = BET_RES_NONE;
	} break;
	case BET_COMP_RND_WEIGHTED: {
		bet_finish_comp(bet, ctx, node_index);
		// *res = BET_RES_NONE;
	} break;
	default: {
	} break;
	}
}

void
bet_deco_tick(
	struct bet *bet,
	struct bet_ctx *ctx,
	usize node_index,
	enum bet_res *res,
	i32 i,
	void *userdata)
{
	struct bet_node *node         = bet_get_node(bet, node_index);
	struct bet_node_ctx *node_ctx = ctx->bet_node_ctx + node_index;
	dbg_assert(node->type == BET_NODE_DECO);
	dbg_assert(node->children_count == 1);
	enum bet_deco_type type = node->sub_type;

	switch(type) {
	case BET_DECO_INVERT: {
		bet_set_child(bet, ctx, node_index, i);
	} break;
	case BET_DECO_FAILURE: {
		bet_set_child(bet, ctx, node_index, i);
	} break;
	case BET_DECO_SUCCESS: {
		bet_set_child(bet, ctx, node_index, i);
	} break;
	case BET_DECO_ONE_SHOT: {
		if(node_ctx->has_run) {
			u8 child_index                 = node->children[0];
			struct bet_node_ctx *child_ctx = ctx->bet_node_ctx + node_index;
			*res                           = child_ctx->res;
			bet_finish_comp(bet, ctx, node_index);
		} else {
			node_ctx->has_run = true;
			bet_set_child(bet, ctx, node_index, i);
		}
	} break;
	case BET_DECO_REPEAT_X_TIMES: {
		bet_set_child(bet, ctx, node_index, i);
	} break;
	case BET_DECO_REPEAT_RND_TIMES: {
		bet_set_child(bet, ctx, node_index, i);
	} break;
	case BET_DECO_REPEAT_UNTIL_SUCCESS: {
		bet_set_child(bet, ctx, node_index, i);
	} break;
	case BET_DECO_REPEAT_UNTIL_FAILURE: {
		bet_set_child(bet, ctx, node_index, i);
	} break;
	default: {
		dbg_sentinel("bet");
	} break;
	}

error:
	return;
}

enum bet_res
bet_action_tick(
	struct bet *bet,
	struct bet_ctx *ctx,
	usize node_index,
	void *userdata)
{
	struct bet_node *node         = bet_get_node(bet, node_index);
	struct bet_node_ctx *node_ctx = ctx->bet_node_ctx + node_index;
	dbg_assert(node->type == BET_NODE_ACTION);
	enum bet_res res = BET_RES_NONE;
	if(ctx->action_do != NULL) {
		res           = ctx->action_do(bet, ctx, node, userdata);
		node_ctx->res = res;
		dbg_assert(res != BET_RES_NONE);
	}

	return res;
}

void
bet_comp_end(
	struct bet *bet,
	struct bet_ctx *ctx,
	usize node_index,
	enum bet_res *res,
	void *userdata)
{
	struct bet_node *node         = bet_get_node(bet, node_index);
	struct bet_node_ctx *node_ctx = ctx->bet_node_ctx + node_index;
	dbg_assert(node->type == BET_NODE_COMP);
	enum bet_comp_type type = node->sub_type;

	switch(type) {
	case BET_COMP_SEQUENCE: {
	} break;
	case BET_COMP_SELECTOR: {
	} break;
	case BET_COMP_PARALLEL: {
		if(*res != BET_RES_RUNNING) {
			// If all nodes finished running, check how many succed and
			// if they are > then the threshold return success;
			i32 acc = 0;
			for(usize i = 0; i < node->children_count; ++i) {
				u8 child_index                 = node->children[i];
				struct bet_node_ctx *child_ctx = ctx->bet_node_ctx + node_index;
				if(child_ctx->res == BET_RES_SUCCESS) {
					acc++;
				}
			}
			i32 value = bet_prop_f32_get(node->props[0], 0);
			if(acc >= value) {
				*res = BET_RES_SUCCESS;
			} else {
				*res = BET_RES_FAILURE;
			}
		}
	} break;
	case BET_COMP_RND: {
	} break;
	case BET_COMP_RND_WEIGHTED: {
	} break;
	default: {
	} break;
	}
}

void
bet_deco_end(
	struct bet *bet,
	struct bet_ctx *ctx,
	usize node_index,
	enum bet_res *res,
	void *userdata)
{
	struct bet_node *node         = bet_get_node(bet, node_index);
	struct bet_node_ctx *node_ctx = ctx->bet_node_ctx + node_index;
	dbg_assert(node->type == BET_NODE_DECO);
	dbg_assert(node->children_count == 1);
	enum bet_deco_type type = node->sub_type;
	node_ctx->run_count++;

	switch(type) {
	case BET_DECO_INVERT: {
		if(*res == BET_RES_SUCCESS) {
			*res = BET_RES_FAILURE;
		} else if(*res == BET_RES_FAILURE) {
			*res = BET_RES_SUCCESS;
		} else {
			dbg_sentinel("bet");
		}
	} break;
	case BET_DECO_FAILURE: {
		*res = BET_RES_FAILURE;
	} break;
	case BET_DECO_SUCCESS: {
		*res = BET_RES_SUCCESS;
	} break;
	case BET_DECO_ONE_SHOT: {
	} break;
	case BET_DECO_REPEAT_X_TIMES:
	case BET_DECO_REPEAT_RND_TIMES: {
		// If the run count is less than the prop trap in running
		if(*res == BET_RES_FAILURE) {
			*res = BET_RES_FAILURE;
		} else {
			if(node_ctx->run_count < node_ctx->run_max) {
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
		dbg_not_implemeneted("bet");
	} break;
	}

error:
	return;
}

static inline void
bet_set_index(struct bet *bet, struct bet_ctx *ctx, u8 node_index)
{
	struct bet_node *node        = bet_get_node(bet, node_index);
	ctx->current                 = node_index;
	struct bet_node_ctx *new_ctx = ctx->bet_node_ctx + ctx->current;
	if(new_ctx->res != BET_RES_RUNNING) {
		new_ctx->i = -1;
	}
}

static inline void
bet_set_child(struct bet *bet, struct bet_ctx *ctx, u8 node_index, i32 i)
{
	struct bet_node *node = bet_get_node(bet, node_index);
	usize child_index     = node->children[i];
	bet_set_index(bet, ctx, child_index);
}

static inline void
bet_finish_comp(struct bet *bet, struct bet_ctx *ctx, u8 node_index)
{
	struct bet_node *node         = bet_get_node(bet, node_index);
	struct bet_node_ctx *node_ctx = ctx->bet_node_ctx + node_index;
	node_ctx->i                   = node->children_count;
}

struct bet_node *
bet_get_node(struct bet *bet, ssize node_index)
{
	dbg_assert(bet != NULL);
	dbg_assert(node_index > 0 && node_index < arr_len(bet->nodes));
	return bet->nodes + node_index;
}

i32
bet_find_child(struct bet *bet, u8 parent_index, u8 child_index)
{
	struct bet_node *parent = bet_get_node(bet, parent_index);

	for(usize i = 0; i < parent->children_count; ++i) {
		if(parent->children[i] == child_index) {
			// Found child
			return i;
		}
	}
	return -1;
}

b32
bet_node_push_child(struct bet_node *parent, usize parent_index, struct bet_node *child, ssize child_index)
{
	dbg_assert(parent != NULL);
	dbg_assert(parent->type == BET_NODE_COMP || parent->type == BET_NODE_DECO);
	dbg_assert(parent->children_count + 1 < MAX_BET_CHILDREN);

	parent->children[parent->children_count++] = child_index;
	child->parent                              = parent_index;
	child->i                                   = parent->children_count - 1;

	return true;
}

b32
bet_node_push_prop(struct bet_node *node, struct bet_prop prop)
{
	dbg_assert(node->prop_count + 1 <= MAX_BET_NODE_PROPS);
	node->props[node->prop_count++] = prop;
	return true;
}

f32
bet_prop_f32_get(struct bet_prop prop, f32 fallback)
{
	f32 res = 0;
	dbg_assert(prop.type == BET_PROP_F32 || prop.type == BET_PROP_NONE);
	res = prop.type == BET_PROP_F32 ? prop.f32 : fallback;

	return res;
}

i32
bet_prop_i32_get(struct bet_prop prop, i32 fallback)
{
	i32 res = 0;
	dbg_assert(prop.type == BET_PROP_I32 || prop.type == BET_PROP_NONE);
	res = prop.type == BET_PROP_I32 ? prop.i32 : fallback;

	return res;
}

b32
bet_prop_bool32_get(struct bet_prop prop)
{
	b32 res = false;
	dbg_assert(prop.type == BET_PROP_BOOL32 || prop.type == BET_PROP_NONE);
	res = prop.b32;
	return res;
}

static inline b32
bet_node_parent_is_parallel(struct bet *bet, struct bet_ctx *ctx, u8 node_index)
{
	b32 res               = false;
	struct bet_node *node = bet_get_node(bet, node_index);

	b32 has_parallel_parent = false;
	u8 parent_index         = node->parent;

	if(parent_index > 0) {
		struct bet_node *parent_node    = bet_get_node(bet, parent_index);
		struct bet_node_ctx *parent_ctx = ctx->bet_node_ctx + parent_index;
		if(
			parent_node->type == BET_NODE_COMP &&
			parent_node->sub_type == BET_COMP_PARALLEL) {
			return true;
		}
	}

	return res;
}
