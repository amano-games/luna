#pragma once

#include "sys-types.h"
#include "sys-log.h"
#include "mem.h"

#include <stdarg.h>

typedef u32 str_match_flags;

enum {
	str_match_flag_case_insensitive  = (1 << 0),
	str_match_flag_right_side_sloppy = (1 << 1),
	str_match_flag_slash_insensitive = (1 << 2),
};

bool32 char_is_space(u8 c);
bool32 char_is_upper(u8 c);
bool32 char_is_lower(u8 c);
bool32 char_is_alpha(u8 c);
bool32 char_is_slash(u8 c);
bool32 char_is_digit(u8 c, u32 base);
u8 char_to_lower(u8 c);
u8 char_to_upper(u8 c);
u8 char_to_correct_slash(u8 c);

// C-String mesurement
u64 cstr8_len(u8 *c);

// String Constructors
#define str8_lit(S)         string8((u8 *)(S), sizeof(S) - 1)
#define str8_array(S, C)    string8((u8 *)(S), sizeof(*(S)) * (C))
#define str8_array_fixed(S) string8((u8 *)(S), sizeof(S))

str8 string8(u8 *str, u64 size);
str8 str8_range(u8 *first, u8 *one_past_last);
str8 str8_zero(void);
str8 str8_cstr(char *c);
str8 str8_cstr_cappend(void *cstr, void *cap);

bool32 str8_match(str8 a, str8 b, str_match_flags flags);

void str8_cpy(str8 *a, str8 *b);
str8 str8_cpy_push(struct alloc alloc, str8 src);
str8 str8_cat_push(struct alloc alloc, str8 s1, str8 s2);
str8 str8_fmtv_push(struct alloc alloc, char *fmt, va_list args);
str8 str8_fmt_push(struct alloc alloc, char *fmt, ...);

f32 str8_to_f32(str8 str);

#define S_(x)   #x
#define S(x)    S_(x)
#define PBWIDTH 20
#define PBCHAR  '#'

void
ascii_progress_bar(u8 percent)
{
	if(SYS_LOG_LEVEL > 2) {
		char pbstr[PBWIDTH];
		mset(pbstr, PBCHAR, PBWIDTH);
		sys_printf("[%-" S(PBWIDTH) ".*s] %u%%", percent * PBWIDTH / 100, pbstr, percent);
		sys_printf(" ");
	}
}
