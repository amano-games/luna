#pragma once

#include "base/dbg.h"
#include "base/str.h"
#include "sys/sys-io.h"
#include "base/types.h"

struct ser_reader {
	char *data;
	int len;
	int cur;
	int depth;
};

struct ser_writer {
	void *f;
};

enum ser_value_type {
	SER_TYPE_ERROR,
	SER_TYPE_END,
	SER_TYPE_OBJECT,
	SER_TYPE_ARRAY,
	SER_TYPE_U8,
	SER_TYPE_I32,
	SER_TYPE_F32,
	SER_TYPE_BOOL,
	SER_TYPE_STRING,
};

struct ser_value {
	u8 type;
	union {
		str8 str;
		u8 u8;
		i32 i32;
		f32 f32;
		b32 b32;
		i32 depth;
	};
};

void
ser_write(struct ser_writer *w, struct ser_value val)
{
	// write tag byte
	sys_file_w(w->f, &val.type, 1);
	enum ser_value_type type = (enum ser_value_type)val.type;
	// write value
	switch(type) {
	case SER_TYPE_U8: {
		sys_file_w(w->f, &val.u8, sizeof(val.u8));
	} break;
	case SER_TYPE_I32: {
		sys_file_w(w->f, &val.i32, sizeof(val.i32));
	} break;
	case SER_TYPE_F32: {
		sys_file_w(w->f, &val.f32, sizeof(val.f32));
	} break;
	case SER_TYPE_STRING: {
		sys_file_w(w->f, &val.str.size, sizeof(val.str.size)); // write length
		sys_file_w(w->f, val.str.str, val.str.size);           // write string
		u8 null = 0;
		sys_file_w(w->f, &null, 1); // write null-terminator
	} break;
	case SER_TYPE_BOOL: {
		sys_file_w(w->f, &val.b32, 1);
	} break;
	default: {
	}
	}
}

void
ser_write_u8(struct ser_writer *w, u8 value)
{
	ser_write(w, (struct ser_value){.type = SER_TYPE_U8, .u8 = value});
}

void
ser_write_i32(struct ser_writer *w, i32 value)
{
	ser_write(w, (struct ser_value){.type = SER_TYPE_I32, .i32 = value});
}

void
ser_write_f32(struct ser_writer *w, f32 value)
{
	ser_write(w, (struct ser_value){.type = SER_TYPE_F32, .f32 = value});
}

void
ser_write_bool(struct ser_writer *w, b32 value)
{
	ser_write(w, (struct ser_value){.type = SER_TYPE_BOOL, .b32 = value});
}

void
ser_write_string(struct ser_writer *w, str8 value)
{
	ser_write(w, (struct ser_value){.type = SER_TYPE_STRING, .str = value});
}

void
ser_write_object(struct ser_writer *w)
{
	ser_write(w, (struct ser_value){.type = SER_TYPE_OBJECT});
}

void
ser_write_array(struct ser_writer *w)
{
	ser_write(w, (struct ser_value){.type = SER_TYPE_ARRAY});
}

void
ser_write_end(struct ser_writer *w)
{
	ser_write(w, (struct ser_value){.type = SER_TYPE_END});
}

bool
ser_safe_read(struct ser_reader *r, void *dst, int size)
{
	int idx = r->cur + size;
	if(idx > r->len) { return false; }
	if(dst) { memcpy(dst, &r->data[r->cur], size); }
	r->cur = idx;
	return true;
}

struct ser_value
ser_read(struct ser_reader *r)
{
	struct ser_value res = {0};
	// read type
	bool ok                  = ser_safe_read(r, &res.type, 1);
	enum ser_value_type type = (enum ser_value_type)res.type;
	// read value
	switch(type) {
	case SER_TYPE_END:
		r->depth--;
		break;
	case SER_TYPE_OBJECT:
	case SER_TYPE_ARRAY:
		r->depth++;
		res.depth = r->depth;
		break;
	case SER_TYPE_U8:
		ok &= ser_safe_read(r, &res.u8, sizeof(res.u8));
		break;
	case SER_TYPE_I32:
		ok &= ser_safe_read(r, &res.i32, sizeof(res.i32));
		break;
	case SER_TYPE_F32:
		ok &= ser_safe_read(r, &res.f32, sizeof(res.f32));
		break;
	case SER_TYPE_BOOL:
		ok &= ser_safe_read(r, &res.b32, 1);
		break;
	case SER_TYPE_STRING:
		ok &= ser_safe_read(r, &res.str.size, sizeof(res.str.size));
		res.str.str = (u8 *)&r->data[r->cur];
		ok &= ser_safe_read(r, NULL, res.str.size);
		uint8_t null;
		ok &= ser_safe_read(r, &null, 1);
		ok &= (null == 0);
		break;
	default: // bad type
		ok &= false;
	}
	// return value, or error if anything above failed
	return ok ? res : (struct ser_value){.type = SER_TYPE_ERROR};
}

void
ser_discard_until_depth(struct ser_reader *r, int depth)
{
	struct ser_value v = {.type = SER_TYPE_END};
	while(v.type != SER_TYPE_ERROR && r->depth != depth) {
		v = ser_read(r);
	}
}

b32
ser_iter_object(struct ser_reader *r, struct ser_value obj, struct ser_value *key, struct ser_value *val)
{
	ser_discard_until_depth(r, obj.depth);
	*key = ser_read(r);
	if(key->type == SER_TYPE_END) { return false; }
	*val = ser_read(r);
	return true;
}

bool
ser_iter_array(struct ser_reader *r, struct ser_value arr, struct ser_value *val)
{
	ser_discard_until_depth(r, arr.depth);
	*val = ser_read(r);
	return val->type != SER_TYPE_END;
}

void
ser_indent_push(int depth, struct str8_list *list, struct alloc alloc)
{
	for(int i = 0; i < depth; i++) {
		str8_list_push(alloc, list, str8_lit("  "));
	}
}

void
ser_value_push_str8_list(struct ser_reader *r, struct ser_value val, int depth, struct str8_list *list, struct alloc alloc)
{
	struct ser_value k, v;
	int count                = 0;
	enum ser_value_type type = (enum ser_value_type)val.type;
	switch(type) {
	case SER_TYPE_OBJECT: {
		str8_list_push(alloc, list, str8_lit("{\n"));
		while(ser_iter_object(r, val, &k, &v)) {
			dbg_assert(k.type == SER_TYPE_STRING);
			if(count++ > 0) { str8_list_push(alloc, list, str8_lit(",\n")); }
			ser_indent_push(depth + 1, list, alloc);
			ser_value_push_str8_list(r, k, depth + 1, list, alloc);
			str8_list_push(alloc, list, str8_lit(": "));
			ser_value_push_str8_list(r, v, depth + 1, list, alloc);
		}
		if(count > 0) { str8_list_push(alloc, list, str8_lit("\n")); }
		ser_indent_push(depth, list, alloc);
		str8_list_push(alloc, list, str8_lit("}"));
	} break;
	case SER_TYPE_ARRAY: {
		str8_list_push(alloc, list, str8_lit("[\n"));
		while(ser_iter_array(r, val, &v)) {
			if(count++ > 0) { str8_list_push(alloc, list, str8_lit(",\n")); }
			ser_indent_push(depth + 1, list, alloc);
			ser_value_push_str8_list(r, v, depth + 1, list, alloc);
		}
		if(count > 0) { str8_list_push(alloc, list, str8_lit("\n")); }
		ser_indent_push(depth, list, alloc);
		str8_list_push(alloc, list, str8_lit("]"));
	} break;
	case SER_TYPE_U8: {
		str8_list_pushf(alloc, list, "%d", val.u8);
	} break;
	case SER_TYPE_I32: {
		str8_list_pushf(alloc, list, "%" PRId32 "", val.i32);
	} break;
	case SER_TYPE_F32: {
		str8_list_pushf(alloc, list, "%.14g", (double)val.f32);
	} break;
	case SER_TYPE_BOOL: {
		str8_list_push(alloc, list, val.b32 ? str8_lit("true") : str8_lit("false"));
	} break;
	case SER_TYPE_STRING: {
		str8_list_push(alloc, list, str8_lit("\""));
		for(usize i = 0; i < val.str.size; i++) {
			if(val.str.str[i] == '"') {
				str8_list_push(alloc, list, str8_lit("\\\""));
			} else {
				str8_list_pushf(alloc, list, "%c", val.str.str[i]);
			}
		}
		str8_list_push(alloc, list, str8_lit("\""));
	} break;

	default: {
		dbg_sentinel("Ser");
	} break;
	}

error:
	return;
}

str8
ser_value_to_str(struct alloc alloc, struct ser_reader *r, struct ser_value value)
{
	struct str8_list list = {0};
	ser_value_push_str8_list(r, value, 0, &list, alloc);
	str8 res = str8_list_join(alloc, &list, NULL);
	return res;
}
