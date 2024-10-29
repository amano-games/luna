#pragma once

#include "sys-assert.h"
#include "sys-io.h"
#include "sys-log.h"
#include "sys-types.h"

struct ser_reader {
	char *data;
	int len;
	int cur;
	int depth;
};

struct ser_writer {
	void *f;
};

enum set_value_type {
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
		bool32 bool32;
		i32 depth;
	};
};

void
ser_write(struct ser_writer *w, struct ser_value val)
{
	// write tag byte
	sys_file_w(w->f, &val.type, 1);
	// write value
	switch(val.type) {
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
		sys_file_w(w->f, &val.bool32, 1);
	} break;
	default: {
	}
	}
}

void
ser_write_u8(struct ser_writer *w, u8 u8)
{
	ser_write(w, (struct ser_value){.type = SER_TYPE_U8, .u8 = u8});
}

void
ser_write_i32(struct ser_writer *w, i32 i32)
{
	ser_write(w, (struct ser_value){.type = SER_TYPE_I32, .i32 = i32});
}

void
ser_write_f32(struct ser_writer *w, f32 f32)
{
	ser_write(w, (struct ser_value){.type = SER_TYPE_F32, .f32 = f32});
}

void
ser_write_bool(struct ser_writer *w, bool32 bool32)
{
	ser_write(w, (struct ser_value){.type = SER_TYPE_BOOL, .bool32 = bool32});
}

void
ser_write_string(struct ser_writer *w, str8 str)
{
	ser_write(w, (struct ser_value){.type = SER_TYPE_STRING, .str = str});
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
	struct ser_value res;
	// read type
	bool ok = ser_safe_read(r, &res.type, 1);
	// read value
	switch(res.type) {
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
		ok &= ser_safe_read(r, &res.bool32, 1);
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

bool32
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
ser_print_indent(int depth)
{
	for(int i = 0; i < depth; i++) {
		sys_printf("    ");
	}
}

void
ser_print_value(struct ser_reader *r, struct ser_value val, int depth)
{
	struct ser_value k, v;
	int count = 0;
	switch(val.type) {
	case SER_TYPE_OBJECT:
		sys_printf("{\n");
		while(ser_iter_object(r, val, &k, &v)) {
			if(count++ > 0) { sys_printf(",\n"); }
			ser_print_indent(depth + 1);
			ser_print_value(r, k, depth + 1);
			sys_printf(": ");
			ser_print_value(r, v, depth + 1);
		}
		if(count > 0) { sys_printf("\n"); }
		ser_print_indent(depth);
		sys_printf("}");
		break;
	case SER_TYPE_ARRAY:
		sys_printf("[\n");
		while(ser_iter_array(r, val, &v)) {
			if(count++ > 0) { sys_printf(",\n"); }
			ser_print_indent(depth + 1);
			ser_print_value(r, v, depth + 1);
		}
		if(count > 0) { sys_printf("\n"); }
		ser_print_indent(depth);
		sys_printf("]");
		break;
	case SER_TYPE_U8:
		sys_printf("%d", val.u8);
		break;
	case SER_TYPE_I32:
		sys_printf("%" PRId32 "", val.i32);
		break;
	case SER_TYPE_F32:
		sys_printf("%.14g", (double)val.f32);
		break;
	case SER_TYPE_BOOL:
		sys_printf(val.bool32 ? "true" : "false");
		break;
	case SER_TYPE_STRING:
		sys_printf("\"");
		for(usize i = 0; i < val.str.size; i++) {
			if(val.str.str[i] == '"') {
				sys_printf("\\\"");
			} else {
				sys_printf("%c", val.str.str[i]);
			}
		}
		sys_printf("\"");
		break;
	default: // bad type
		sys_printf("Ser Error: Bad type %d", val.type);
		return;
	}
}
