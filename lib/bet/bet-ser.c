#include "bet-ser.h"

#include "base/arr.h"
#include "base/dbg.h"
#include "lib/serialize/serialize.h"
#include "base/str.h"
#include "sys/sys-io.h"
#include "base/log.h"
#include "base/utils.h"

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
	bet_read(&r, &res, alloc);

	return res;
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
	case BET_PROP_STR: {
		str8 str = str8_cstr((char *)prop.str);
		ser_write_string(w, str);
	} break;
	case BET_PROP_BOOL32: {
		ser_write_i32(w, prop.b32);
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
bet_write(struct ser_writer *w, struct bet *bet)
{
	ser_write_object(w);
	ser_write_string(w, str8_lit("node_count"));
	ser_write_i32(w, arr_len(bet->nodes));
	ser_write_string(w, str8_lit("nodes"));
	ser_write_array(w);
	for(usize i = 0; i < arr_len(bet->nodes); i++) {
		bet_node_write(w, bet->nodes[i]);
	}
	ser_write_end(w);
	ser_write_end(w);
}

struct bet_prop
bet_prop_read(struct ser_reader *r, struct ser_value obj)
{
	struct bet_prop res = {0};
	struct ser_value key, value;
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
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
			case BET_PROP_STR: {
				str8 dst = str8_cstr((char *)res.str);
				str8_cpy(&value.str, &dst);
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
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	struct bet_node res = {0};
	struct ser_value key, value;

	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
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

i32
bet_read(
	struct ser_reader *r,
	struct bet *bet,
	struct alloc alloc)
{
	struct ser_value root = ser_read(r);
	dbg_assert(root.type == SER_TYPE_OBJECT);
	struct ser_value key, value;
	while(ser_iter_object(r, root, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("node_count"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			bet->node_count = value.i32;
			bet->nodes      = arr_new(bet->nodes, bet->node_count, alloc);
		} else if(str8_match(key.str, str8_lit("nodes"), 0)) {
			dbg_assert(value.type == SER_TYPE_ARRAY);
			dbg_assert(bet->nodes != NULL);
			struct ser_value item = {0};
			while(ser_iter_array(r, value, &item)) {
				arr_push(bet->nodes, bet_node_read(r, item));
			}
			dbg_assert((size)arr_len(bet->nodes) == bet->node_count);
			dbg_assert(bet->nodes[0].type == BET_NODE_NONE);
		}
	}

	return bet->node_count;
}
