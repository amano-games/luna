#pragma once

#include "jsmn.h"
#include "sys-io.h"
#include "sys-types.h"
#include "str.h"
#include "sys-assert.h"

bool32
json_load(const str8 path, struct alloc alloc, str8 *out)
{
	void *f = sys_file_open_r(path);
	if(!f) {
		log_warn("JSON", "Can't open %s\n", path.str);
		return 0;
	}
	sys_file_seek_end(f, 0);
	i32 size = sys_file_tell(f);
	sys_file_seek_set(f, 0);
	u8 *buf = (u8 *)alloc.allocf(alloc.ctx, (usize)size + 1);
	if(!buf) {
		sys_file_close(f);
		log_error("JSON", "loading %s", path.str);
		return 0;
	}
	sys_file_r(f, buf, size);
	sys_file_close(f);
	buf[size] = '\0';
	out->str  = buf;
	out->size = size;
	return 1;
}

static i32
json_eq(str8 json, jsmntok_t *tok, str8 b)
{
	str8 a = {.str = json.str + tok->start, .size = tok->end - tok->start};

	if(tok->type == JSMN_STRING && str8_match(a, b, 0)) {
		return 0;
	}
	return -1;
}

static i32
json_parse_i32(str8 json, jsmntok_t *tok)
{
	i32 num = str8_to_i32((str8){
		.str  = (u8 *)json.str + tok->start,
		.size = tok->end - tok->start,
	});

	return num;
}

static f32
json_parse_f32(str8 json, jsmntok_t *tok)
{
	// TODO: Replace with sys_parse_string to use sscanf when it's not broken
	f32 num = str8_to_f32((str8){
		.str  = (u8 *)json.str + tok->start,
		.size = tok->end - tok->start,
	});
	return num;
}

static str8
json_str8_cpy_push(str8 json, jsmntok_t *tok, struct alloc alloc)
{
	str8 src = (str8){
		.str  = (u8 *)json.str + tok->start,
		.size = tok->end - tok->start,
	};
	str8 res = str8_cpy_push(alloc, src);
	return res;
}

static void
json_str8_cpy(str8 json, jsmntok_t *tok, str8 *dst)
{
	str8 src = (str8){
		.str  = (u8 *)json.str + tok->start,
		.size = tok->end - tok->start,
	};
	str8_cpy(&src, dst);
}

static inline str8
json_str8(str8 json, jsmntok_t *tok)
{
	str8 res = (str8){
		.str  = (u8 *)json.str + tok->start,
		.size = tok->end - tok->start,
	};
	return res;
}

static inline usize
json_obj_count(str8 json, const jsmntok_t *tok)
{
	assert(tok->type == JSMN_OBJECT);
	usize res = 0;
	jsmn_parser parser;
	jsmn_init(&parser);
	str8 obj = {
		.str  = json.str + tok->start,
		.size = tok->end - tok->start,
	};
	res = jsmn_parse(&parser, (const char *)obj.str, obj.size, NULL, 0);
	return res;
}
