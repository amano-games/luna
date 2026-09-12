#pragma once

#include "base/date-time.h"
#include "base/types.h"
#include "base/mem.h"

#include <stdarg.h>

struct str8_node {
	struct str8_node *next;
	str8 str;
};

struct str8_meta_node {
	struct str8_meta_node *next;
	struct str8_node *node;
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

b32 char_is_space(u8 c);
b32 char_is_upper(u8 c);
b32 char_is_lower(u8 c);
b32 char_is_alpha(u8 c);
b32 char_is_slash(u8 c);
b32 char_is_digit(u8 c, u32 base);
b32 char_is_ascii(u8 c);
b32 char_is_utf8(u8 c);

u8 char_to_lower(u8 c);
u8 char_to_upper(u8 c);
u8 char_to_correct_slash(u8 c);

// C-String mesurement
usize cstr8_len(u8 *c);

// String Constructors
#define str8_lit(S)         string8((u8 *)(S), sizeof(S) - 1)
#define str8_lit_comp(S)    {(u8 *)(S), sizeof(S) - 1}
#define str8_array(S, C)    string8((u8 *)(S), sizeof(*(S)) * (C))
#define str8_array_fixed(S) string8((u8 *)(S), sizeof(S))
#define str8_spread(S)      (int)(S).size, (S).str

str8 string8(u8 *str, u64 size);
str8 str8_range(u8 *first, u8 *one_past_last);
str8 str8_zero(void);
str8 str8_cstr(char *c);
str8 str8_cstr_cappend(void *cstr, void *cap);

b32 str8_ends_with(str8 str, str8 end, str_match_flags flags);
b32 str8_starts_with(str8 str, str8 start, str_match_flags flags);
b32 str8_match(str8 a, str8 b, str_match_flags flags);
usize str8_find_needle(str8 str, usize start_pos, str8 needle, str_match_flags flags);
usize str8_find_needle_reverse(str8 str, usize start_pos, str8 needle, str_match_flags flags);

void str8_cpy(str8 *a, str8 *b);
str8 str8_cpy_push(struct alloc alloc, str8 src);
str8 str8_cat_push(struct alloc alloc, str8 s1, str8 s2);
void str8_cat_in_place(str8 *dst, str8 *src);
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
str8 str8_skip_chop_slashes(str8 str);

// Path helpers
str8 str8_chop_last_slash(str8 str);
str8 str8_skip_last_slash(str8 str);
str8 str8_chop_last_dot(str8 str);
str8 str8_skip_last_dot(str8 str);

i32 str8_to_i32(str8 str);
f32 str8_to_f32(str8 str);
b32 str8_to_bool32(str8 str);
u64 str8_to_u64(str8 str, u32 radix);
i64 str8_to_i64(str8 str, u32 radix);

// String Stylization

str8 str8_to_upper(struct alloc alloc, str8 str);
str8 str8_to_lower(struct alloc alloc, str8 str);
str8 str8_to_backslashed(struct alloc alloc, str8 str);

// String List Construction Functions

struct str8_node *str8_list_push_node(struct str8_list *list, struct str8_node *node);
struct str8_node *str8_list_push(struct alloc alloc, struct str8_list *list, str8 str);

struct str8_node *str8_list_pushf(struct alloc alloc, struct str8_list *list, char *fmt, ...);
void str8_list_concat_in_place(struct str8_list *list, struct str8_list *to_push);

// String Splitting & Joining
#define str8_list_first(list) ((list)->first ? (list)->first->str : str8_zero())
struct str8_list str8_split(struct alloc alloc, str8 str, u8 *split_chars, usize split_char_count, str_split_flags flags);
struct str8_list str8_split_by_string_chars(struct alloc alloc, str8 str, str8 split_chars, str_split_flags flags);
str8 str8_list_join(struct alloc alloc, struct str8_list *list, struct str_join *optional_params);

// Blob builder: append into a str8_list, flatten with str8_serial_end.
void str8_serial_begin(struct alloc alloc, struct str8_list *srl);
str8 str8_serial_end(struct alloc alloc, struct str8_list *srl);
void str8_serial_write_to_dst(struct str8_list *srl, void *out);
u64 str8_serial_push_align(struct alloc alloc, struct str8_list *srl, u64 align);
void *str8_serial_push_size(struct alloc alloc, struct str8_list *srl, u64 size);
void *str8_serial_push_data(struct alloc alloc, struct str8_list *srl, void *data, u64 size);
void str8_serial_push_data_list(struct alloc alloc, struct str8_list *srl, struct str8_node *first);
void *str8_serial_push_u64(struct alloc alloc, struct str8_list *srl, u64 x);
void *str8_serial_push_u32(struct alloc alloc, struct str8_list *srl, u32 x);
void *str8_serial_push_u16(struct alloc alloc, struct str8_list *srl, u16 x);
void *str8_serial_push_u8(struct alloc alloc, struct str8_list *srl, u8 x);
void *str8_serial_push_cstr(struct alloc alloc, struct str8_list *srl, str8 str);
void *str8_serial_push_string(struct alloc alloc, struct str8_list *srl, str8 str);
#define str8_serial_push_array(alloc, srl, ptr, count) str8_serial_push_data((alloc), (srl), (ptr), sizeof(*(ptr)) * (count))
#define str8_serial_push_struct(alloc, srl, ptr)       str8_serial_push_array((alloc), (srl), (ptr), 1)

// Read packed fields from a str8 at an offset.
u64 str8_deserial_read(str8 string, u64 off, void *read_dst, u64 read_size, u64 granularity);
void *str8_deserial_get_raw_ptr(str8 string, u64 off, u64 size);
u64 str8_deserial_read_cstr(str8 string, u64 off, str8 *cstr_out);
u64 str8_deserial_read_block(str8 string, u64 off, u64 size, str8 *block_out);
#define str8_deserial_read_array(string, off, ptr, count) str8_deserial_read((string), (off), (ptr), sizeof(*(ptr)) * (count), sizeof(*(ptr)))
#define str8_deserial_read_struct(string, off, ptr)       str8_deserial_read_array(string, off, ptr, 1)

struct str8_list wrapped_lines_from_str(struct alloc alloc, str8 str, usize first_line_max_width, usize max_width, usize wrap_indent);

str8 str_from_week_day(enum week_day week_day);
str8 str_from_month(enum month month);
str8 str_date_time_push(struct alloc alloc, struct date_time *date_time);

static enum os_kind str8_to_os(str8 str);
static str8 str8_from_os(enum os_kind value);
