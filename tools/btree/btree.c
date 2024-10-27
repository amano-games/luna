#include "btree.h"
#include "arr.h"
#include "json.h"
#include "mem-arena.h"
#include "mem.h"
#include "str.h"
#include "sys-io.h"
#include "sys-log.h"
#include "sys-assert.h"
#include "sys-types.h"
#include "sys.h"
#include "tools/utils.h"
#include <stdio.h>

#define AI_FILE_EXT "bet"

struct prop_res
handle_prop(str8 json, jsmntok_t *tokens, i32 index, struct bet *bet, struct alloc scratch)
{
	jsmntok_t *root = &tokens[index];
	assert(root->type == JSMN_OBJECT);
	str8 node_json = {
		.str  = json.str + root->start,
		.size = root->end - root->start,
	};
	jsmn_parser parser;
	jsmn_init(&parser);
	struct prop_res res = {0};
	res.token_count     = jsmn_parse(&parser, (const char *)node_json.str, node_json.size, NULL, 0);

	for(usize i = index + 1; i < index + res.token_count; i++) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];
		if(json_eq(json, key, str8_lit("value")) == 0) {
			switch(value->type) {
			case JSMN_ARRAY: {
				res.prop.type = BET_PROP_I32_ARR;
				for(int j = 0; j < value->size; j++) {
					jsmntok_t *num = &tokens[i + j + 2];
					assert(num->type == JSMN_PRIMITIVE);
					res.prop.i32_arr[j] = json_parse_i32(json, num);
				}
			} break;
			case JSMN_PRIMITIVE: {
				char *first = (char *)json.str + value->start;
				if(*first != 'n') {
					res.prop.type = BET_PROP_F32;
					res.prop.f32  = json_parse_f32(json, value);
				}
			} break;
			default: {
				// NOT_IMPLEMENTED;
			} break;
			}
		}
	}
	return res;
}

struct node_res
handle_node(str8 json, jsmntok_t *tokens, i32 index, struct bet *bet, struct alloc scratch)
{
	jsmntok_t *root = &tokens[index];
	assert(root->type == JSMN_OBJECT);
	str8 node_json = {
		.str  = json.str + root->start,
		.size = root->end - root->start,
	};
	jsmn_parser parser;
	jsmn_init(&parser);
	struct node_res res = {0};
	res.token_count     = jsmn_parse(&parser, (const char *)node_json.str, node_json.size, NULL, 0);

	for(usize i = index + 1; i < index + res.token_count; i++) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];
		if(json_eq(json, key, str8_lit("type")) == 0) {
			if(json_eq(json, value, str8_lit("Selector")) == 0) {
				res.node_index = bet_push_node(bet, (struct bet_node){
														.type     = BET_NODE_COMP,
														.sub_type = BET_COMP_SELECTOR,
													});
			} else if(json_eq(json, value, str8_lit("Sequence")) == 0) {
				res.node_index = bet_push_node(bet, (struct bet_node){
														.type     = BET_NODE_COMP,
														.sub_type = BET_COMP_SEQUENCE,
													});
			} else if(json_eq(json, value, str8_lit("Random")) == 0) {
				res.node_index = bet_push_node(bet, (struct bet_node){
														.type     = BET_NODE_COMP,
														.sub_type = BET_COMP_RND,
													});
			} else if(json_eq(json, value, str8_lit("Random Weights")) == 0) {
				res.node_index = bet_push_node(bet, (struct bet_node){
														.type     = BET_NODE_COMP,
														.sub_type = BET_COMP_RND_WEIGHTED,
													});
			} else if(json_eq(json, value, str8_lit("Invert")) == 0) {
				res.node_index = bet_push_node(bet, (struct bet_node){
														.type     = BET_NODE_DECO,
														.sub_type = BET_DECO_INVERT,
													});
			} else if(json_eq(json, value, str8_lit("Failure")) == 0) {
				res.node_index = bet_push_node(bet, (struct bet_node){
														.type     = BET_NODE_DECO,
														.sub_type = BET_DECO_FAILURE,
													});
			} else if(json_eq(json, value, str8_lit("Success")) == 0) {
				res.node_index = bet_push_node(bet, (struct bet_node){
														.type     = BET_NODE_DECO,
														.sub_type = BET_DECO_SUCCESS,
													});
			} else if(json_eq(json, value, str8_lit("Repeat X times")) == 0) {
				res.node_index = bet_push_node(bet, (struct bet_node){
														.type     = BET_NODE_DECO,
														.sub_type = BET_DECO_REPEAT_X_TIMES,
													});
			} else if(json_eq(json, value, str8_lit("One shot")) == 0) {
				res.node_index = bet_push_node(bet, (struct bet_node){
														.type     = BET_NODE_DECO,
														.sub_type = BET_DECO_ONE_SHOT,
													});
			} else if(json_eq(json, value, str8_lit("Repeat Until Failure")) == 0) {
				res.node_index = bet_push_node(bet, (struct bet_node){
														.type     = BET_NODE_DECO,
														.sub_type = BET_DECO_REPEAT_UNTIL_FAILURE,
													});
			} else if(json_eq(json, value, str8_lit("Repeat Until Success")) == 0) {
				res.node_index = bet_push_node(bet, (struct bet_node){
														.type     = BET_NODE_DECO,
														.sub_type = BET_DECO_REPEAT_UNTIL_SUCCESS,
													});
			} else {
				struct bet_node node = {
					.type = BET_NODE_ACTION,
				};
				mcpy(node.note, json.str + value->start, value->end - value->start);
				usize len      = cstr8_len((u8 *)node.note);
				node.note[len] = '\0';

				res.node_index = bet_push_node(bet, node);
			}
		} else if(json_eq(json, key, str8_lit("properties")) == 0) {
			struct bet_prop props[MAX_BET_NODE_PROPS] = {0};
			assert(value->size <= MAX_BET_NODE_PROPS);
			for(i32 j = 0; j < value->size; j++) {
				i32 prop_index  = i + 2;
				jsmntok_t *item = &tokens[prop_index];
				assert(item->type == JSMN_OBJECT);
				struct prop_res prop_res = handle_prop(json, tokens, prop_index, bet, scratch);
				bet_push_prop(bet, res.node_index, prop_res.prop);
				i += prop_res.token_count;
			}
		} else if(json_eq(json, key, str8_lit("decorators")) == 0) {
			assert(value->type == JSMN_ARRAY);
			assert(res.node_index != 0);
			if(value->size > 0) {
				log_warn("ai-gen", "using decorators, not supported");
			}

		} else if(json_eq(json, key, str8_lit("childNodes")) == 0) {
			assert(res.node_index != 0);
			struct bet_node *node = &bet->nodes[res.node_index];
			assert(node->type != BET_NODE_NONE);
			assert(node->type == BET_NODE_COMP || node->type == BET_NODE_DECO);

			assert(value->type == JSMN_ARRAY);
			for(i32 j = 0; j < value->size; j++) {
				i32 child_index = i + 2;
				jsmntok_t *item = &tokens[child_index];
				assert(item->type == JSMN_OBJECT);
				struct node_res child_res = handle_node(json, tokens, child_index, bet, scratch);
				bet_push_child(bet, res.node_index, child_res.node_index);
				i += child_res.token_count;
			}
		}
	}

	return res;
}

void
handle_json(str8 json, struct bet *bet, struct alloc scratch)
{
	jsmn_parser parser;
	jsmn_init(&parser);
	i32 token_count = jsmn_parse(&parser, (char *)json.str, json.size, NULL, 0);
	jsmn_init(&parser);
	jsmntok_t *tokens = arr_ini(token_count, sizeof(jsmntok_t), scratch);
	i32 json_res      = jsmn_parse(&parser, (char *)json.str, json.size, tokens, token_count);
	assert(json_res == token_count);

	jsmntok_t root = tokens[0];

	assert(root.type == JSMN_OBJECT);

	handle_node(json, tokens, 0, bet, scratch);
}

int
handle_btree(str8 in_path, str8 out_path)
{
	usize mem_size = MMEGABYTE(1);
	u8 *mem_buffer = sys_alloc(NULL, mem_size);
	assert(mem_buffer != NULL);
	// sys_printf("processing %s ...", in_path.str);

	struct marena marena = {0};
	marena_init(&marena, mem_buffer, mem_size);

	struct alloc alloc = marena_allocator(&marena);
	str8 json          = {0};
	json_load(in_path, alloc, &json);

	struct bet bet = {0};
	bet_init(&bet);

	handle_json(json, &bet, alloc);

	for(usize i = 1; i < bet.count; ++i) {
		str8 node_str = bet_node_serialize(&bet, i, alloc);
		// sys_printf("%s", node_str.str);
	}

	char out_file_path_buff[FILENAME_MAX];
	str8 out_file_path = str8_array_fixed(out_file_path_buff);
	change_extension((char *)out_path.str, (char *)out_file_path.str, AI_FILE_EXT);

	void *out_file;
	if(!(out_file = sys_file_open_w(out_file_path))) {
		log_error("ai-gen", "can't open file %s for writing!", out_file_path.str);
		return -1;
	}

	if(sys_file_w(out_file, &bet, sizeof(struct bet)) != 1) {
		log_error("ai-gen", "failed to write file %s", out_file_path.str);
		return -1;
	}

	sys_file_close(out_file);

	sys_free(mem_buffer);
	// sys_printf("%s -> %s\n", in_path.str, out_file_path.str);
	log_info("ai-gen", "%s -> %s\n", in_path.str, out_file_path.str);
	return 1;
}
