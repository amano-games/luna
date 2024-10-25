#include "bet.h"
#include "mem.h"
#include "rndm.h"
#include "sys-io.h"
#include "sys-log.h"
#include "sys-types.h"
#include "sys-assert.h"
#include "sys-utils.h"

bool32 bet_ctx_handle_node_res(struct bet *bet, struct bet_ctx *bet_ctx, u8 node_index, enum bet_res res);

enum bet_res bet_tick_comp(struct bet *bet, struct bet_ctx *ctx, usize node_index, void *userdata);
enum bet_res bet_tick_action(struct bet *bet, struct bet_ctx *ctx, usize node_index, void *userdata);
enum bet_res bet_tick_deco(struct bet *bet, struct bet_ctx *ctx, usize node_index, void *userdata);

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

bool32
bet_push_child(struct bet *bet, usize parent_index, usize child_index)
{
	struct bet_node *parent = bet_get_node(bet, parent_index);
	assert(parent != NULL);
	assert(parent->type == BET_NODE_COMP || parent->type == BET_NODE_DECO);
	assert(parent->children_count + 1 < MAX_BET_CHILDREN);

	parent->children[parent->children_count++] = child_index;
	bet->nodes[child_index].parent             = parent_index;
	bet->nodes[child_index].i                  = parent->children_count - 1;

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
bet_tick(struct bet *bet, struct bet_ctx *ctx, void *userdata)
{
	enum bet_res res = BET_RES_NONE;
	if(bet->count == 0) { return res; }

	usize node_index = MAX(ctx->current, 1);

	for(;;) {
		if(node_index == 0) {
			return res;
		}

		// Tick the current node
		struct bet_node *current = bet_get_node(bet, node_index);
		res                      = bet_tick_node(bet, ctx, node_index, userdata);

		// If running return inmediatly, tick_node takes care of storing the first running node
		if(res == BET_RES_RUNNING) {
			return res;
		} else {
			// If not running clear the saved node
			ctx->current = 0;
		}

		// When the node is not running loop back to parents and check siblings when neccesary
		for(;;) {
			struct bet_node *current = bet_get_node(bet, node_index);
			usize parent_index       = current->parent;
			// We arrived at root
			if(parent_index == 0) {
				ctx->i       = 0;
				ctx->current = 0;
				return res;
			}

			struct bet_node *parent     = bet_get_node(bet, parent_index);
			bool32 should_check_sibling = false;
			if(parent->type == BET_NODE_COMP) {
				enum bet_comp_type type = parent->sub_type;
				switch(type) {
				case BET_COMP_SELECTOR:
				case BET_COMP_SEQUENCE: {
					// Is not the last child of the parent
					should_check_sibling = current->i < parent->children_count - 1;
				} break;
				default: {
				} break;
				}
			}
			node_index = parent_index;

			if(should_check_sibling) {
				// Go up in the tree
				// and check the next child
				ctx->i = current->i + 1;
				break;
			} else {
				// Go up in the tree
				// And set the current
				// index that should be checked to the parent index
				ctx->i = parent->i;
			}
		}
	}

	return res;
}

enum bet_res
bet_tick_node(struct bet *bet, struct bet_ctx *ctx, usize node_index, void *userdata)
{
	sys_printf("tick: %d", (int)node_index);
	enum bet_res res      = BET_RES_NONE;
	struct bet_node *node = bet_get_node(bet, node_index);

	switch(node->type) {
	case BET_NODE_COMP: {
		res = bet_tick_comp(bet, ctx, node_index, userdata);
	} break;
	case BET_NODE_ACTION: {
		res = bet_tick_action(bet, ctx, node_index, userdata);
	} break;
	case BET_NODE_DECO: {
		res = bet_tick_deco(bet, ctx, node_index, userdata);
	} break;
	default: {
		NOT_IMPLEMENTED;
	} break;
	}
	bet_ctx_handle_node_res(bet, ctx, node_index, res);

	return res;
}

enum bet_res
bet_tick_sequence(struct bet *bet, struct bet_ctx *ctx, usize node_index, usize initial, void *userdata)
{
	struct bet_node *node = bet_get_node(bet, node_index);
	assert(node->type == BET_NODE_COMP);
	assert(node->sub_type == BET_COMP_SEQUENCE);
	enum bet_res res = BET_RES_NONE;

	for(usize i = initial; i < node->children_count; ++i) {
		usize child_index      = node->children[i];
		struct bet_node *child = bet_get_node(bet, child_index);
		res                    = bet_tick_node(bet, ctx, child_index, userdata);
		if(res != BET_RES_SUCCESS) {
			return res;
		}
	}
	return res;
}

enum bet_res
bet_tick_selector(struct bet *bet, struct bet_ctx *ctx, usize node_index, usize initial, void *userdata)
{
	struct bet_node *node = bet_get_node(bet, node_index);
	assert(node->type == BET_NODE_COMP);
	assert(node->sub_type == BET_COMP_SELECTOR);
	enum bet_res res = BET_RES_NONE;

	for(usize i = initial; i < node->children_count; ++i) {
		usize child_index      = node->children[i];
		struct bet_node *child = bet_get_node(bet, child_index);
		res                    = bet_tick_node(bet, ctx, child_index, userdata);
		if(res != BET_RES_FAILURE) {
			return res;
		}
	}
	return res;
}

enum bet_res
bet_tick_rnd(struct bet *bet, struct bet_ctx *ctx, usize node_index, void *userdata)
{
	struct bet_node *node = bet_get_node(bet, node_index);
	assert(node->type == BET_NODE_COMP);
	assert(node->sub_type == BET_COMP_RND);
	enum bet_res res = BET_RES_NONE;

	usize rnd         = rndm_range_i32(0, node->children_count - 1);
	usize child_index = node->children[rnd];
	return bet_tick_node(bet, ctx, child_index, userdata);
}

enum bet_res
bet_tick_rnd_weighted(struct bet *bet, struct bet_ctx *ctx, usize node_index, void *userdata)
{
	struct bet_node *node = bet_get_node(bet, node_index);
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
	usize rnd         = rndm_weighted_choice_i32(choices, node->children_count);
	usize child_index = node->children[rnd];
	return bet_tick_node(bet, ctx, child_index, userdata);
}

enum bet_res
bet_tick_action(struct bet *bet, struct bet_ctx *ctx, usize node_index, void *userdata)
{
	struct bet_node *node = bet_get_node(bet, node_index);
	assert(node->type == BET_NODE_ACTION);
	enum bet_res res = BET_RES_NONE;
	res              = ctx->action_do(bet, ctx, node, userdata);
	sys_printf("  res: %s", BET_RES_STR[res]);
	assert(res != BET_RES_NONE);

	return res;
}

enum bet_res
bet_tick_deco(struct bet *bet, struct bet_ctx *ctx, usize node_index, void *userdata)
{
	struct bet_node *node = bet_get_node(bet, node_index);
	assert(node->type == BET_NODE_DECO);
	assert(node->children_count == 1);
	enum bet_deco_type type       = node->sub_type;
	enum bet_res res              = BET_RES_NONE;
	usize child_index             = node->children[0];
	res                           = bet_tick_node(bet, ctx, child_index, userdata);
	struct bet_node_ctx *node_ctx = ctx->bet_node_ctx + child_index;

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
	case BET_DECO_ONE_SHOT: {
		if(node_ctx->run_count > 1) { res = BET_RES_FAILURE; }
	} break;
	case BET_DECO_REPEAT_X_TIMES: {
		struct bet_prop prop = node->props[0];
		assert(prop.type == BET_PROP_F32);
		// If the run count is less than the prop trap in running
		if((i32)node_ctx->run_count >= (i32)prop.f32) {
			// But only if the child is not running
			if(res != BET_RES_RUNNING) {
				res = BET_RES_RUNNING;
			}
		}
		sys_printf("  deco-run-x: %d,%d -> %s", (i32)node_ctx->run_count, (i32)prop.f32, BET_RES_STR[res]);

	} break;
	case BET_DECO_REPEAT_UNTIL_SUCCESS: {
		if(res != BET_RES_SUCCESS) {
			res = BET_RES_RUNNING;
		}
	} break;
	case BET_DECO_REPEAT_UNTIL_FAILURE: {
		if(res != BET_RES_FAILURE) {
			res = BET_RES_RUNNING;
		}
	} break;
	default: {
		NOT_IMPLEMENTED;
	} break;
	}
	return res;
}

enum bet_res
bet_tick_comp(struct bet *bet, struct bet_ctx *ctx, usize node_index, void *userdata)
{
	struct bet_node *node = bet_get_node(bet, node_index);
	assert(node->type == BET_NODE_COMP);
	enum bet_res res        = BET_RES_NONE;
	enum bet_comp_type type = node->sub_type;
	usize initial           = ctx->i;
	ctx->i                  = 0;

	switch(type) {
	case BET_COMP_SEQUENCE: {
		sys_printf("-> %d", ctx->i);
		return bet_tick_sequence(bet, ctx, node_index, initial, userdata);
	} break;
	case BET_COMP_SELECTOR: {
		sys_printf("? %d", ctx->i);
		return bet_tick_selector(bet, ctx, node_index, initial, userdata);
	} break;
	case BET_COMP_RND: {
		sys_printf("rnd");
		return bet_tick_rnd(bet, ctx, node_index, userdata);
	} break;
	case BET_COMP_RND_WEIGHTED: {
		sys_printf("rnd weighted");
		return bet_tick_rnd_weighted(bet, ctx, node_index, userdata);
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

bool32
bet_ctx_handle_node_res(struct bet *bet, struct bet_ctx *ctx, u8 node_index, enum bet_res res)
{
	if(res == BET_RES_RUNNING && ctx->current == 0) {
		sys_printf("Store current: %d", node_index);
		struct bet_node *node = bet_get_node(bet, node_index);
		ctx->i                = node->i;
		ctx->current          = node_index;
	}
	return true;
}
