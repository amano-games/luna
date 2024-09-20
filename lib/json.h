#pragma once

#include "jsmn.h"
#include "sys-log.h"
#include "sys-str.h"
#include "sys-types.h"
#include "str.h"

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
	char *end = (char *)json.str + (tok->end - tok->start);
	i32 num   = 0;
	sys_parse_string((char *)json.str + tok->start, "%d", &num);

	return num;
}

static f32
json_parse_f32(str8 json, jsmntok_t *tok)
{
	char *end = (char *)json.str + (tok->end - tok->start);
	// TODO: Replace with sys_parse_string to use sscanf when it's not broken
	f32 num = str8_to_f32((str8){
		.str  = (u8 *)json.str + tok->start,
		.size = tok->end - tok->start,
	});
	return num;
}
