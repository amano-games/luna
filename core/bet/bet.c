#include "bet.h"
#include "arr.h"
#include "mem.h"
#include "rndm.h"
#include "serialize/serialize.h"
#include "str.h"
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
bet_init(struct bet *bet, struct alloc alloc)
{
	bet->nodes = arr_ini(1, sizeof(*bet->nodes), alloc);
	bet->alloc = alloc;
	arr_push(bet->nodes, (struct bet_node){0});
}

i32
bet_push_node(struct bet *bet, struct bet_node node)
{
	assert(bet != NULL);
	assert(arr_len(bet->nodes) + 1 < MAX_BET_NODES);
	arr_push_packed(bet->nodes, node, bet->alloc);
	return arr_len(bet->nodes) - 1;
}

struct bet_node *
bet_get_node(struct bet *bet, usize node_index)
{
	assert(bet != NULL);
	assert(node_index > 0 && node_index < arr_len(bet->nodes));
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
	if(arr_len(bet->nodes) == 0) { return res; }

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
				case BET_COMP_SELECTOR: {
					if(res != BET_RES_SUCCESS) {
						// Is not the last child of the parent
						should_check_sibling = current->i < parent->children_count - 1;
					}
				} break;
				case BET_COMP_SEQUENCE: {
					if(res == BET_RES_SUCCESS) {
						// Is not the last child of the parent
						should_check_sibling = current->i < parent->children_count - 1;
					}
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
	if(ctx->debug) {
		sys_printf("tick: %d", (int)node_index);
	}
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
		log_error("Bet", "node type not supported type: %d node_index: %d", node->type, (int)node_index);
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
	assert(weights.type == BET_PROP_U8_ARR);

	for(usize i = 0; i < node->children_count; ++i) {
		choices[i].key   = i;
		choices[i].value = weights.u8_arr[i];
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
	if(ctx->debug) {
		sys_printf("  res: %s", BET_RES_STR[res]);
	}
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
		if(ctx->debug) {
			sys_printf("  deco-invert: %s", BET_RES_STR[res]);
		}
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
		if(ctx->debug) {
			sys_printf("  deco-run-x: %d,%d -> %s", (i32)node_ctx->run_count, (i32)prop.f32, BET_RES_STR[res]);
		}

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
		if(ctx->debug) {
			sys_printf("-> %d", ctx->i);
		}
		return bet_tick_sequence(bet, ctx, node_index, initial, userdata);
	} break;
	case BET_COMP_SELECTOR: {
		if(ctx->debug) {
			sys_printf("? %d", ctx->i);
		}
		return bet_tick_selector(bet, ctx, node_index, initial, userdata);
	} break;
	case BET_COMP_RND: {
		if(ctx->debug) {
			sys_printf("rnd");
		}
		return bet_tick_rnd(bet, ctx, node_index, userdata);
	} break;
	case BET_COMP_RND_WEIGHTED: {
		if(ctx->debug) {
			sys_printf("rnd weighted");
		}
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
bet_load(str8 path, struct alloc alloc, struct alloc scratch)
{
	log_info("Bet", "Load bet %s", path.str);
	struct bet res                    = {0};
	struct sys_full_file_res file_res = sys_load_full_file(path, scratch);
	char *data                        = file_res.data;
	usize size                        = file_res.size;

	if(data == NULL) {
		log_error("Bet", "failed to open bet file: %s", path.str);
		return res;
	}

	struct ser_reader r = {
		.data = data,
		.len  = size,
	};
	struct ser_value arr = ser_read(&r);
	// ser_print_value(&r, arr, 0);
	bet_init(&res, alloc);
	bet_read(&r, arr, &res, MAX_BET_NODES);

	return res;
}

bool32
bet_ctx_handle_node_res(struct bet *bet, struct bet_ctx *ctx, u8 node_index, enum bet_res res)
{
	if(res == BET_RES_RUNNING && ctx->current == 0) {
		if(ctx->debug) {
			sys_printf("Store current: %d", node_index);
		}
		struct bet_node *node = bet_get_node(bet, node_index);
		ctx->i                = node->i;
		ctx->current          = node_index;
	}
	return true;
}

void
bet_prop_write(struct ser_writer *w, struct bet_prop prop)
{
	ser_write_object(w);
	ser_write_string(w, str8_lit("type"));
	ser_write_i32(w, prop.type);
	ser_write_string(w, str8_lit("value"));
	switch(prop.type) {
	case BET_PROP_NONE: {
	} break;
	case BET_PROP_F32: {
		ser_write_f32(w, prop.f32);
	} break;
	case BET_PROP_I32: {
		ser_write_i32(w, prop.i32);
	} break;
	case BET_PROP_U8_ARR: {
		ser_write_array(w);
		for(usize i = 0; i < ARRLEN(prop.u8_arr); i++) {
			ser_write_u8(w, prop.u8_arr[i]);
		}
		ser_write_end(w);
	} break;
	case BET_PROP_BOOL32: {
		ser_write_i32(w, prop.bool32);
	} break;
	}
	ser_write_end(w);
}

void
bet_node_write(struct ser_writer *w, struct bet_node n)
{
	ser_write_object(w);
	ser_write_string(w, str8_lit("type"));
	ser_write_i32(w, n.type);
	ser_write_string(w, str8_lit("sub_type"));
	ser_write_i32(w, n.sub_type);
	ser_write_string(w, str8_lit("parent"));
	ser_write_u8(w, n.parent);
	ser_write_string(w, str8_lit("i"));
	ser_write_u8(w, n.i);

	ser_write_string(w, str8_lit("children_count"));
	ser_write_u8(w, n.children_count);
	ser_write_string(w, str8_lit("children"));
	ser_write_array(w);
	for(usize i = 0; i < n.children_count; ++i) {
		ser_write_u8(w, n.children[i]);
	}
	ser_write_end(w);

	ser_write_string(w, str8_lit("prop_count"));
	ser_write_u8(w, n.prop_count);
	ser_write_string(w, str8_lit("props"));
	ser_write_array(w);
	for(usize i = 0; i < n.prop_count; ++i) {
		bet_prop_write(w, n.props[i]);
	}
	ser_write_end(w);

	ser_write_string(w, str8_lit("name"));
	ser_write_string(w, str8_cstr(n.name));
	ser_write_end(w);
}

void
bet_nodes_write(struct ser_writer *w, struct bet_node *nodes, usize count)
{
	ser_write_array(w);
	for(usize i = 0; i < count; i++) {
		bet_node_write(w, nodes[i]);
	}
	ser_write_end(w);
}

struct bet_prop
bet_prop_read(struct ser_reader *r, struct ser_value obj)
{
	struct bet_prop res = {0};
	struct ser_value key, value;
	while(ser_iter_object(r, obj, &key, &value)) {
		assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("type"), 0)) {
			res.type = value.i32;
		} else if(str8_match(key.str, str8_lit("value"), 0)) {
			switch(res.type) {
			case BET_PROP_NONE: {
			} break;
			case BET_PROP_F32: {
				res.f32 = value.f32;
			} break;
			case BET_PROP_I32: {
				res.i32 = value.i32;
			} break;
			case BET_PROP_U8_ARR: {
				struct ser_value item_val;
				usize i = 0;
				while(ser_iter_array(r, value, &item_val) && i < ARRLEN(res.u8_arr)) {
					res.u8_arr[i] = item_val.u8;
					i++;
				}
			} break;
			case BET_PROP_BOOL32: {
				res.i32 = value.i32;
			} break;
			}
		}
	}

	return res;
}

struct bet_node
bet_node_read(struct ser_reader *r, struct ser_value obj)
{
	struct bet_node res = {0};
	struct ser_value key, value;

	while(ser_iter_object(r, obj, &key, &value)) {
		assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("type"), 0)) {
			res.type = value.i32;
		} else if(str8_match(key.str, str8_lit("sub_type"), 0)) {
			res.sub_type = value.i32;
		} else if(str8_match(key.str, str8_lit("parent"), 0)) {
			res.parent = value.u8;
		} else if(str8_match(key.str, str8_lit("i"), 0)) {
			res.i = value.u8;
		} else if(str8_match(key.str, str8_lit("children_count"), 0)) {
			res.children_count = value.u8;
		} else if(str8_match(key.str, str8_lit("children"), 0)) {
			struct ser_value item_val;
			usize i = 0;
			while(ser_iter_array(r, value, &item_val) && i < ARRLEN(res.children)) {
				res.children[i] = item_val.u8;
				i++;
			}
		} else if(str8_match(key.str, str8_lit("prop_count"), 0)) {
			res.prop_count = value.u8;
		} else if(str8_match(key.str, str8_lit("props"), 0)) {
			struct ser_value item_val;
			int i = 0;
			while(ser_iter_array(r, value, &item_val) && i < MAX_BET_CHILDREN) {
				res.props[i] = bet_prop_read(r, item_val);
				i++;
			}
		} else if(str8_match(key.str, str8_lit("name"), 0)) {
			mcpy(res.name, value.str.str, value.str.size);
		}
	}

	return res;
}

// reads up to `max_count` rects into the `rects` array and returns the number
// of rects read — note: if you have a dynamic array as part of your base
// library, it could be used here instead of the fixed `rects` array
i32
bet_read(
	struct ser_reader *r,
	struct ser_value arr,
	struct bet *bet,
	usize max_count)
{
	struct ser_value val;
	usize i = 0;
	while(ser_iter_array(r, arr, &val) && i < max_count) {
		bet_push_node(bet, bet_node_read(r, val));
		i++;
	}
	return i;
}
