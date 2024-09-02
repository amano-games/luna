#pragma once

#include "jsmn.h"
#include "sys-log.h"
#include "sys-str.h"
#include "sys-types.h"
#include "str.h"

static i32
json_eq(const char *json, jsmntok_t *tok, const char *s)
{
	if(
		tok->type == JSMN_STRING &&
		(i32)str_len(s) == tok->end - tok->start &&
		str_cmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}

static i32
json_parse_i32(char *json, jsmntok_t *tok)
{
	char *end = json + (tok->end - tok->start);
	i32 num   = 0;
	sys_parse_string(json + tok->start, "%d", &num);

	return num;
}

static f32
json_parse_f32(char *json, jsmntok_t *tok)
{
	char *end = json + (tok->end - tok->start);
	// TODO: Replace with sys_parse_string to use sscanf when it's not broken
	f32 num = str_to_f32((string){
		.data = json + tok->start,
		.len  = tok->end - tok->start,
	});
	return num;
}
