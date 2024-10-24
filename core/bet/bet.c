#include "bet.h"
#include "mem.h"
#include "rndm.h"
#include "sys-io.h"
#include "sys-log.h"
#include "sys-types.h"
#include "sys-assert.h"
#include "sys-utils.h"

bool32 bet_ctx_handle_node_res(struct bet_ctx *bet_ctx, u8 node_index, enum bet_res res);
void bet_enqueue_init(struct bet_queue *queue);
bool32 bet_enqueue(struct bet_queue *queue, u8 value);
u8 bet_dequeue(struct bet_queue *queue);
u8 bet_queue_peek(struct bet_queue *queue);
bool32 bet_queue_is_empty(struct bet_queue *queue);
bool32 bet_queue_is_full(struct bet_queue *queue);
void bet_queue_print(struct bet_queue *queue);

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

usize
bet_get_next_node(struct bet *bet, struct bet_ctx *ctx, usize current_index)
{
	usize parent_index      = MAX(bet_queue_peek(&ctx->running_queue), 1);
	struct bet_node *parent = bet_get_node(bet, parent_index);

	assert(parent->children_count > 0);

	bool32 is_root       = parent_index == 1;
	i32 child_index      = bet_find_child(bet, parent_index, current_index);
	bool32 is_last_child = child_index == parent->children_count - 1;

	if(is_root && is_last_child) {
		return 0;
	}
	if(is_last_child) {
		bet_dequeue(&ctx->running_queue);
		return bet_get_next_node(bet, ctx, parent_index);
	} else {
		usize res = parent->children[child_index + 1];
		return res;
	}

	return 0;
}

enum bet_res
bet_tick(struct bet *bet, struct bet_ctx *ctx, void *userdata)
{
	enum bet_res res = BET_RES_NONE;
	if(bet->count == 0) { return res; }

	// Make sure at least we run the root (1)
	usize current_index = bet_queue_peek(&ctx->running_queue);
	if(current_index == 0) {
		current_index = 1;
	}

	while(current_index > 0) {
		res = bet_tick_node(bet, ctx, current_index, userdata);

		if(res != BET_RES_RUNNING) {
			// TODO: Maybe it would be simpler to use a stack instead of a queue
			// Check the parent and start from there
			if(!bet_queue_is_empty(&ctx->running_queue)) {
				assert(bet_dequeue(&ctx->running_queue) == current_index);
				current_index = bet_get_next_node(bet, ctx, current_index);
				if(current_index != 0) {
					bet_enqueue_init(&ctx->running_queue);
				}
			}
		} else {
			current_index = 0;
		}
	}

	return res;
}

enum bet_res
bet_tick_node(struct bet *bet, struct bet_ctx *ctx, usize node_index, void *userdata)
{
	enum bet_res res      = BET_RES_NONE;
	struct bet_node *node = bet_get_node(bet, node_index);

	// NOTE: Clear memory
	for(usize i = 0; i < node->children_count; ++i) {
		usize child_index                        = node->children[i];
		ctx->bet_node_ctx[child_index].run_count = 0;
	}

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
	bet_ctx_handle_node_res(ctx, node_index, res);

	return res;
}

enum bet_res
bet_tick_sequence(struct bet *bet, struct bet_ctx *ctx, usize node_index, void *userdata)
{
	struct bet_node *node = bet_get_node(bet, node_index);
	assert(node->type == BET_NODE_COMP);
	assert(node->sub_type == BET_COMP_SEQUENCE);
	enum bet_res res = BET_RES_NONE;

	for(usize i = 0; i < node->children_count; ++i) {
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
bet_tick_selector(struct bet *bet, struct bet_ctx *ctx, usize node_index, void *userdata)
{
	struct bet_node *node = bet_get_node(bet, node_index);
	assert(node->type == BET_NODE_COMP);
	assert(node->sub_type == BET_COMP_SELECTOR);
	enum bet_res res = BET_RES_NONE;

	for(usize i = 0; i < node->children_count; ++i) {
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
		choices[i].key   = node->children[i];
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

	switch(type) {
	case BET_COMP_SEQUENCE: {
		sys_printf("->");
		return bet_tick_sequence(bet, ctx, node_index, userdata);
	} break;
	case BET_COMP_SELECTOR: {
		sys_printf("?");
		return bet_tick_selector(bet, ctx, node_index, userdata);
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
bet_ctx_handle_node_res(struct bet_ctx *ctx, u8 node_index, enum bet_res res)
{
	u8 current_running = bet_queue_peek(&ctx->running_queue);
	if(res != BET_RES_RUNNING) {
		ctx->bet_node_ctx[node_index].run_count++;
	}

	if(current_running != node_index) {
		if(res == BET_RES_RUNNING && node_index > 1) {
			bet_enqueue(&ctx->running_queue, node_index);
		}
	}

	return true;
}

bool32
bet_queue_is_full(struct bet_queue *queue)
{
	return queue->count == MAX_BET_CHILDREN;
}

bool32
bet_queue_is_empty(struct bet_queue *queue)
{
	return queue->count == 0;
}

void
bet_enqueue_init(struct bet_queue *queue)
{
	if(!bet_queue_is_empty(queue)) {
		for(int i = 0; i < queue->count; i++) {
			int index          = (queue->head + i) % MAX_BET_CHILDREN;
			queue->data[index] = 0;
		}
	}
	queue->head  = 0;
	queue->tail  = 0;
	queue->count = 0;
}

bool32
bet_enqueue(struct bet_queue *queue, u8 value)
{
	assert(!bet_queue_is_full(queue));

	queue->data[queue->tail] = value;                                // Add the value at the tail
	queue->tail              = (queue->tail + 1) % MAX_BET_CHILDREN; // Circular increment of tail
	queue->count++;                                                  // Increase the size of the queue
	// bet_queue_print(queue);
	return true;
}

u8
bet_dequeue(struct bet_queue *queue)
{
	if(bet_queue_is_empty(queue)) {
		return 0;
	}

	u8 value                 = queue->data[queue->head];             // Get the value at the head
	queue->data[queue->head] = 0;                                    // clear the value
	queue->head              = (queue->head + 1) % MAX_BET_CHILDREN; // Circular increment of head
	queue->count--;                                                  // Decrease the size of the queue
	// bet_queue_print(queue);
	return value;
}

u8
bet_queue_peek(struct bet_queue *queue)
{
	if(bet_queue_is_empty(queue)) {
		return 0;
	}

	u8 value = queue->data[queue->head]; // Peek the value at the head
	return value;
}

// Print the queue for debugging purposes
void
bet_queue_print(struct bet_queue *queue)
{
	if(bet_queue_is_empty(queue)) {
		sys_printf("Queue is empty");
		return;
	}

	sys_printf("Queue contents: ");
	for(int i = 0; i < queue->count; i++) {
		int index = (queue->head + i) % MAX_BET_CHILDREN;
		sys_printf("%d ", queue->data[index]);
	}
}
