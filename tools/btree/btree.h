#pragma once

#include "base/link-list.h"
#include "lib/bet/bet.h"
#include "base/types.h"

#define AI_FILE_EXT "bet"

struct prop_res {
	usize token_count;
	struct bet_prop prop;
};

struct node_res {
	usize token_count;
	u8 node_index;
};

struct bet_node_holder {
	struct bet_node *nodes;
};

struct bet_node_list_node {
	struct bet_node_list_node *next;
	struct bet_node bet_node;
};

struct bet_node_list {
	struct bet_node_list_node *first;
	struct bet_node_list_node *last;
	u32 node_count;
};

static inline struct bet_node_list_node *
bet_node_list_push_node(
	struct bet_node_list *list,
	struct bet_node_list_node *node)
{
	SLLQueuePush(list->first, list->last, node);
	list->node_count += 1;
	return (node);
}

static inline struct bet_node_list_node *
bet_node_list_push_node_set_value(
	struct bet_node_list *list,
	struct bet_node_list_node *node,
	struct bet_node bet_node)
{
	SLLQueuePush(list->first, list->last, node);
	list->node_count += 1;
	node->bet_node = bet_node;
	return (node);
}

static inline struct bet_node_list_node *
bet_node_list_push(
	struct alloc alloc,
	struct bet_node_list *list,
	struct bet_node bet_node)
{
	struct bet_node_list_node *node = alloc_struct(alloc, node);
	bet_node_list_push_node_set_value(list, node, bet_node);
	return (node);
}

int handle_btree(str8 in_path, str8 out_path, struct alloc scratch);
