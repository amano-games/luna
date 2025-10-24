#include "btree.h"
#include "base/arr.h"
#include "lib/bet/bet.h"
#include "lib/bet/bet-ser.h"
#include "lib/json.h"
#include "base/marena.h"
#include "base/mem.h"
#include "base/path.h"
#include "lib/serialize/serialize.h"
#include "base/str.h"
#include "sys/sys-io.h"
#include "base/log.h"
#include "base/types.h"
#include "base/utils.h"
#include "sys/sys.h"
#include "base/dbg.h"

static inline i32 bet_node_holder_push(struct bet_node_holder *holder, struct bet_node node, struct alloc alloc);

struct prop_res
handle_prop(str8 json, jsmntok_t *tokens, i32 index, struct alloc scratch)
{
	jsmntok_t *root = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	str8 node_json = {
		.str  = json.str + root->start,
		.size = root->end - root->start,
	};
	jsmn_parser parser;
	jsmn_init(&parser);
	struct prop_res res = {0};
	res.token_count     = jsmn_parse(&parser, (const char *)node_json.str, node_json.size, NULL, 0);

	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];
		if(json_eq(json, key, str8_lit("value")) == 0) {
			switch(value->type) {
			case JSMN_ARRAY: {
				res.prop.type = BET_PROP_U8_ARR;
				dbg_assert((usize)value->size <= ARRLEN(res.prop.u8_arr));
				for(int j = 0; j < value->size; j++) {
					jsmntok_t *num = &tokens[i + j + 2];
					dbg_assert(num->type == JSMN_PRIMITIVE);
					res.prop.u8_arr[j] = (u8)json_parse_i32(json, num);
				}
			} break;
			case JSMN_PRIMITIVE: {
				char *first = (char *)json.str + value->start;
				if(*first == 't' || *first == 'f') {
					res.prop.type = BET_PROP_BOOL32;
					res.prop.b32  = *first == 't' ? true : false;
				} else if(*first != 'n') { // diff from null
					res.prop.type = BET_PROP_F32;
					res.prop.f32  = json_parse_f32(json, value);
				}
			} break;
			case JSMN_STRING: {
				res.prop.type = BET_PROP_STR;
				dbg_assert((usize)value->size <= ARRLEN(res.prop.str));
				str8 str = str8_cstr((char *)res.prop.str);
				json_str8_cpy(json, value, &str);
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
handle_node(str8 json, jsmntok_t *tokens, i32 index, struct bet_node_holder *holder, struct alloc alloc, struct alloc scratch)
{
	jsmntok_t *root = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
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
				res.node_index = bet_node_holder_push(holder, (struct bet_node){
																  .type     = BET_NODE_COMP,
																  .sub_type = BET_COMP_SELECTOR,
															  },
					alloc);
			} else if(json_eq(json, value, str8_lit("Sequence")) == 0) {
				res.node_index = bet_node_holder_push(holder, (struct bet_node){
																  .type     = BET_NODE_COMP,
																  .sub_type = BET_COMP_SEQUENCE,
															  },
					alloc);
			} else if(json_eq(json, value, str8_lit("Parallel")) == 0) {
				res.node_index = bet_node_holder_push(holder, (struct bet_node){
																  .type     = BET_NODE_COMP,
																  .sub_type = BET_COMP_PARALLEL,
															  },
					alloc);
			} else if(json_eq(json, value, str8_lit("Random")) == 0) {
				res.node_index = bet_node_holder_push(holder, (struct bet_node){
																  .type     = BET_NODE_COMP,
																  .sub_type = BET_COMP_RND,
															  },
					alloc);
			} else if(json_eq(json, value, str8_lit("Random Weights")) == 0) {
				res.node_index = bet_node_holder_push(holder, (struct bet_node){
																  .type     = BET_NODE_COMP,
																  .sub_type = BET_COMP_RND_WEIGHTED,
															  },
					alloc);
			} else if(json_eq(json, value, str8_lit("Invert")) == 0) {
				res.node_index = bet_node_holder_push(holder, (struct bet_node){
																  .type     = BET_NODE_DECO,
																  .sub_type = BET_DECO_INVERT,
															  },
					alloc);
			} else if(json_eq(json, value, str8_lit("Failure")) == 0) {
				res.node_index = bet_node_holder_push(holder, (struct bet_node){
																  .type     = BET_NODE_DECO,
																  .sub_type = BET_DECO_FAILURE,
															  },
					alloc);
			} else if(json_eq(json, value, str8_lit("Success")) == 0) {
				res.node_index = bet_node_holder_push(holder, (struct bet_node){
																  .type     = BET_NODE_DECO,
																  .sub_type = BET_DECO_SUCCESS,
															  },
					alloc);
			} else if(json_eq(json, value, str8_lit("Repeat X times")) == 0) {
				res.node_index = bet_node_holder_push(holder, (struct bet_node){
																  .type     = BET_NODE_DECO,
																  .sub_type = BET_DECO_REPEAT_X_TIMES,
															  },
					alloc);
			} else if(json_eq(json, value, str8_lit("Repeat RND times")) == 0) {
				res.node_index = bet_node_holder_push(holder, (struct bet_node){
																  .type     = BET_NODE_DECO,
																  .sub_type = BET_DECO_REPEAT_RND_TIMES,
															  },
					alloc);
			} else if(json_eq(json, value, str8_lit("One shot")) == 0) {
				res.node_index = bet_node_holder_push(holder, (struct bet_node){
																  .type     = BET_NODE_DECO,
																  .sub_type = BET_DECO_ONE_SHOT,
															  },
					alloc);
			} else if(json_eq(json, value, str8_lit("Repeat Until Failure")) == 0) {
				res.node_index = bet_node_holder_push(holder, (struct bet_node){
																  .type     = BET_NODE_DECO,
																  .sub_type = BET_DECO_REPEAT_UNTIL_FAILURE,
															  },
					alloc);
			} else if(json_eq(json, value, str8_lit("Repeat Until Success")) == 0) {
				res.node_index = bet_node_holder_push(holder, (struct bet_node){
																  .type     = BET_NODE_DECO,
																  .sub_type = BET_DECO_REPEAT_UNTIL_SUCCESS,
															  },
					alloc);
			} else {
				struct bet_node node = {
					.type = BET_NODE_ACTION,
				};

				res.node_index = bet_node_holder_push(holder, node, alloc);
			}
			struct bet_node *node = holder->nodes + res.node_index;

			dbg_assert((usize)(value->end - value->start) < ARRLEN(holder->nodes[0].name));

			str8 dst = {
				.str  = (u8 *)node->name,
				.size = ARRLEN(node->name),
			};
			json_str8_cpy(json, value, &dst);

		} else if(json_eq(json, key, str8_lit("label")) == 0) {
			usize len = value->end - value->start;
			if(len > 0) {
				struct bet_node *node = holder->nodes + res.node_index;
				dbg_assert((usize)(value->end - value->start) < ARRLEN(holder->nodes[0].name));

				str8 dst = {
					.str  = (u8 *)node->name,
					.size = ARRLEN(node->name),
				};
				json_str8_cpy(json, value, &dst);
			}
		} else if(json_eq(json, key, str8_lit("properties")) == 0) {
			struct bet_prop props[MAX_BET_NODE_PROPS] = {0};
			dbg_assert(value->size <= MAX_BET_NODE_PROPS);
			for(i32 j = 0; j < value->size; j++) {
				i32 prop_index  = i + 2;
				jsmntok_t *item = &tokens[prop_index];
				dbg_assert(item->type == JSMN_OBJECT);
				struct prop_res prop_res = handle_prop(json, tokens, prop_index, scratch);
				if(prop_res.prop.type != BET_PROP_NONE) {
					bet_node_push_prop(holder->nodes + res.node_index, prop_res.prop);
				}
				i += prop_res.token_count;
			}
		} else if(json_eq(json, key, str8_lit("decorators")) == 0) {
			dbg_assert(value->type == JSMN_ARRAY);
			dbg_assert(res.node_index != 0);
			if(value->size > 0) {
				log_warn("ai-gen", "using decorators, not supported");
			}

		} else if(json_eq(json, key, str8_lit("childNodes")) == 0) {
			dbg_assert(res.node_index != 0);
			struct bet_node *node = &holder->nodes[res.node_index];
			dbg_assert(node->type != BET_NODE_NONE);
			dbg_assert(node->type == BET_NODE_COMP || node->type == BET_NODE_DECO);

			dbg_assert(value->type == JSMN_ARRAY);
			for(i32 j = 0; j < value->size; j++) {
				i32 child_index = i + 2;
				jsmntok_t *item = &tokens[child_index];
				dbg_assert(item->type == JSMN_OBJECT);
				struct node_res child_res = handle_node(json, tokens, child_index, holder, alloc, scratch);
				bet_node_push_child(holder->nodes + res.node_index, res.node_index, holder->nodes + child_res.node_index, child_res.node_index);
				i += child_res.token_count;
			}
		}
	}

	return res;
}

void
handle_btree_json(str8 json, struct bet_node_holder *holder, struct alloc alloc, struct alloc scratch)
{
	jsmn_parser parser;
	jsmn_init(&parser);
	i32 token_count = jsmn_parse(&parser, (char *)json.str, json.size, NULL, 0);
	jsmn_init(&parser);
	jsmntok_t *tokens = arr_new(tokens, token_count, scratch);
	i32 json_res      = jsmn_parse(&parser, (char *)json.str, json.size, tokens, token_count);
	dbg_assert(json_res == token_count);

	jsmntok_t root = tokens[0];

	dbg_assert(root.type == JSMN_OBJECT);

	handle_node(json, tokens, 0, holder, alloc, scratch);
}

int
handle_btree(str8 in_path, str8 out_path, struct alloc scratch)
{
	usize mem_size = MKILOBYTE(100);
	u8 *mem_buffer = sys_alloc(NULL, mem_size);
	dbg_assert(mem_buffer != NULL);
	struct marena marena = {0};
	marena_init(&marena, mem_buffer, mem_size);
	struct alloc alloc = marena_allocator(&marena);

	str8 json = {0};
	json_load(in_path, scratch, &json);

	struct bet_node_holder holder = {0};
	holder.nodes                  = arr_new(holder.nodes, 1, alloc);
	arr_push(holder.nodes, (struct bet_node){0});
	handle_btree_json(json, &holder, alloc, scratch);

	str8 out_file_path = path_make_file_name_with_ext(scratch, out_path, str8_lit(AI_FILE_EXT));

	void *out_file;
	if(!(out_file = sys_file_open_w(out_file_path))) {
		log_error("ai-gen", "can't open file %s for writing!", out_file_path.str);
		return -1;
	}

	struct ser_writer w = {.f = out_file};
	struct bet bet      = {.node_count = arr_len(holder.nodes), .nodes = holder.nodes};
	bet_write(&w, &bet);

	// if(sys_file_w(out_file, &bet, sizeof(struct bet)) != 1) {
	// 	log_error("ai-gen", "failed to write file %s", out_file_path.str);
	// 	return -1;
	// }

	sys_file_close(out_file);

	sys_free(mem_buffer);
	// sys_printf("%s -> %s\n", in_path.str, out_file_path.str);
	log_info("ai-gen", "%s -> %s\n", in_path.str, out_file_path.str);

	return 1;
}

static inline i32
bet_node_holder_push(struct bet_node_holder *holder, struct bet_node node, struct alloc alloc)
{
	dbg_assert(holder != NULL);
	arr_push_packed(holder->nodes, node, alloc);
	return arr_len(holder->nodes) - 1;
}
