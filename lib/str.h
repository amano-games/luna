#pragma once

#include "sys-types.h"
#include "sys-log.h"
#include "mem.h"

#include <stdarg.h>

struct str8_node {
	struct str8_node *next;
	str8 str;
};

struct str8_list {
	struct str8_node *first;
	struct str8_node *last;
	u64 node_count;
	u64 total_size;
};

struct str_join {
	str8 pre;
	str8 sep;
	str8 post;
};

typedef u32 str_match_flags;
enum {
	str_match_flag_case_insensitive  = (1 << 0),
	str_match_flag_right_side_sloppy = (1 << 1),
	str_match_flag_slash_insensitive = (1 << 2),
};

typedef u32 str_split_flags;
enum {
	str_split_flag_keep_empties = (1 << 0),
};

bool32 char_is_space(u8 c);
bool32 char_is_upper(u8 c);
bool32 char_is_lower(u8 c);
bool32 char_is_alpha(u8 c);
bool32 char_is_slash(u8 c);
bool32 char_is_digit(u8 c, u32 base);
bool32 char_is_ascii(u8 c);
bool32 char_is_utf8(u8 c);

u8 char_to_lower(u8 c);
u8 char_to_upper(u8 c);
u8 char_to_correct_slash(u8 c);

// C-String mesurement
usize cstr8_len(u8 *c);

// String Constructors
#define str8_lit(S)         string8((u8 *)(S), sizeof(S) - 1)
#define str8_array(S, C)    string8((u8 *)(S), sizeof(*(S)) * (C))
#define str8_array_fixed(S) string8((u8 *)(S), sizeof(S))

str8 string8(u8 *str, u64 size);
str8 str8_range(u8 *first, u8 *one_past_last);
str8 str8_zero(void);
str8 str8_cstr(char *c);
str8 str8_cstr_cappend(void *cstr, void *cap);

bool32 str8_ends_with(str8 str, str8 end, str_match_flags flags);
bool32 str8_starts_with(str8 str, str8 start, str_match_flags flags);
bool32 str8_match(str8 a, str8 b, str_match_flags flags);
usize str8_find_needle(str8 str, usize start_pos, str8 needle, str_match_flags flags);
usize str8_find_needle_reverse(str8 str, usize start_pos, str8 needle, str_match_flags flags);

void str8_cpy(str8 *a, str8 *b);
str8 str8_cpy_push(struct alloc alloc, str8 src);
str8 str8_cat_push(struct alloc alloc, str8 s1, str8 s2);
str8 str8_fmtv_push(struct alloc alloc, char *fmt, va_list args);
str8 str8_fmt_push(struct alloc alloc, char *fmt, ...);

// String slicing
str8 str8_postfix(str8 str, usize size);
str8 str8_skip(str8 str, usize amt);
str8 str8_substr(str8 str, union rng_u64 range);
str8 str8_prefix(str8 str, usize size);
str8 str8_skip(str8 str, usize amt);
str8 str8_postfix(str8 str, usize size);
str8 str8_chop(str8 str, usize amt);
str8 str8_skip_chop_whitespace(str8 str);

// Path helpers
str8 str8_chop_last_slash(str8 str);
str8 str8_skip_last_slash(str8 str);
str8 str8_chop_last_dot(str8 str);
str8 str8_skip_last_dot(str8 str);

i32 str8_to_i32(str8 str);
f32 str8_to_f32(str8 str);

// String List Construction Functions

struct str8_node *str8_list_pushf(struct alloc alloc, struct str8_list *list, char *fmt, ...);

// String Splitting & Joining
struct str8_list str8_split(struct alloc alloc, str8 str, u8 *split_chars, usize split_char_count, str_split_flags flags);
struct str8_list str8_split_by_string_chars(struct alloc alloc, str8 str, str8 split_chars, str_split_flags flags);
struct str8_list str8_split_path(struct alloc alloc, str8 str);
str8 str8_list_join(struct alloc alloc, struct str8_list *list, struct str_join *optional_params);

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

struct str8_list wrapped_lines_from_str(struct alloc alloc, str8 str, usize first_line_max_width, usize max_width, usize wrap_indent);
