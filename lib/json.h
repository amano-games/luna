#pragma once

#include "jsmn.h"
#include "sys/sys-io.h"
#include "base/types.h"
#include "base/str.h"
#include "base/dbg.h"

enum json_copy_flags {
	json_copy_none     = 0,
	json_copy_unescape = 1 << 0
};

b32
json_load(const str8 path, struct alloc alloc, str8 *out)
{
	void *f = sys_file_open_r(path);
	if(!f) {
		log_warn("JSON", "Can't open %s\n", path.str);
		return 0;
	}
	sys_file_seek_end(f, 0);
	ssize f_size = sys_file_tell(f);
	sys_file_seek_set(f, 0);
	u8 *buf = alloc_arr(alloc, buf, f_size + 1);
	if(!buf) {
		sys_file_close(f);
		log_error("JSON", "loading %s", path.str);
		return 0;
	}
	sys_file_r(f, buf, f_size);
	sys_file_close(f);
	buf[f_size] = '\0';
	out->str    = buf;
	out->size   = f_size;
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
	dbg_assert(tok->type == JSMN_PRIMITIVE);
	i32 res = str8_to_i32((str8){
		.str  = (u8 *)json.str + tok->start,
		.size = tok->end - tok->start,
	});

	return res;
}

static b32
json_parse_bool32(str8 json, jsmntok_t *tok)
{
	dbg_assert(tok->type == JSMN_PRIMITIVE);
	i32 res = str8_to_bool32((str8){
		.str  = (u8 *)json.str + tok->start,
		.size = tok->end - tok->start,
	});

	return res;
}

static f32
json_parse_f32(str8 json, jsmntok_t *tok)
{
	// TODO: Replace with sys_parse_string to use sscanf when it's not broken
	dbg_assert(tok->type == JSMN_PRIMITIVE);
	f32 res = str8_to_f32((str8){
		.str  = (u8 *)json.str + tok->start,
		.size = tok->end - tok->start,
	});
	return res;
}

// Returns number of bytes required for unescaped string.
// If out == NULL: only compute size.
// If out != NULL: write into out->str and set out->size.
static usize
json_unescape_str8(str8 src, str8 *out)
{
	usize res = 0;
	// First pass: compute length if out==NULL
	// Second pass: write data if out!=NULL
	for(usize i = 0; i < src.size; i++) {
		u8 c = src.str[i];
		if(c == '\\' && i + 1 < src.size) {
			i++;
			u8 esc = src.str[i];

			switch(esc) {
			case 'n':
				if(out) out->str[res] = '\n';
				res++;
				break;
			case 't':
				if(out) out->str[res] = '\t';
				res++;
				break;
			case 'r':
				if(out) out->str[res] = '\r';
				res++;
				break;
			case 'b':
				if(out) out->str[res] = '\b';
				res++;
				break;
			case 'f':
				if(out) out->str[res] = '\f';
				res++;
				break;
			case '\\':
				if(out) out->str[res] = '\\';
				res++;
				break;
			case '"':
				if(out) out->str[res] = '"';
				res++;
				break;
			case '/':
				if(out) out->str[res] = '/';
				res++;
				break;

			case 'u':
				dbg_not_implemeneted("json decoding \\uXXXX");
				break;

			default:
				dbg_not_implemeneted("json decoding unknown escape");
				break;
			}
		} else {
			if(out) out->str[res] = c;
			res++;
		}
	}

	if(out) {
		out->size = res;
	}

error:;
	return res;
}

static str8
json_str8_cpy_push(str8 json, jsmntok_t *tok, struct alloc alloc, enum json_copy_flags flags)
{
	dbg_assert(tok->type == JSMN_STRING);
	b32 unescape = (flags & json_copy_unescape);
	str8 src     = (str8){
			.str  = (u8 *)json.str + tok->start,
			.size = tok->end - tok->start,
    };
	str8 res = {0};
	if(!unescape) {
		res = str8_cpy_push(alloc, src);
	} else {
		usize str_size = json_unescape_str8(src, NULL);
		if(str_size > 0) {
			res.str = alloc_size(alloc, str_size + 1, alignof(u8), false);
			json_unescape_str8(src, &res);
			res.str[res.size] = '\0';
		}
	}

	return res;
}

static void
json_str8_cpy(str8 json, jsmntok_t *tok, str8 *dst)
{
	dbg_assert(tok->type == JSMN_STRING);
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
	dbg_assert(tok->type == JSMN_OBJECT);
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
