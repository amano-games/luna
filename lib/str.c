#include "sys-utils.h"
#include "str.h"
#include "sys-utils.h"
#include "sys-assert.h"
#include <string.h>

static u8 INTEGER_SYMBOLS[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

static u8 INTEGER_SYMBOL_REVERSE[128] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

inline bool32
char_is_space(u8 c)
{
	return (c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\f' || c == '\v');
}

inline bool32
char_is_upper(u8 c)
{
	return ('A' <= c && c <= 'Z');
}

inline bool32
char_is_lower(u8 c)
{
	return ('a' <= c && c <= 'z');
}

inline bool32
char_is_alpha(u8 c)
{
	return (char_is_upper(c) || char_is_lower(c));
}

inline bool32
char_is_slash(u8 c)
{
	return (c == '/' || c == '\\');
}

inline bool32
char_is_digit(u8 c, u32 base)
{
	bool32 result = 0;
	if(0 < base && base <= 16) {
		u8 val = INTEGER_SYMBOL_REVERSE[c];
		if(val < base) {
			result = 1;
		}
	}
	return (result);
}

inline u8
char_to_lower(u8 c)
{
	if(char_is_upper(c)) {
		c += ('a' - 'A');
	}
	return (c);
}

inline u8
char_to_upper(u8 c)
{
	if(char_is_lower(c)) {
		c += ('A' - 'a');
	}
	return (c);
}

inline u8
char_to_correct_slash(u8 c)
{
	if(char_is_slash(c)) {
		c = '/';
	}
	return (c);
}

inline u64
cstr8_len(u8 *c)
{
	u8 *p = c;
	for(; *p != 0; p += 1);
	return (p - c);
}

inline str8
string8(u8 *str, u64 size)
{
	str8 result = {str, size};
	return (result);
}

inline str8
str8_range(u8 *first, u8 *one_past_last)
{
	str8 result = {first, (u64)(one_past_last - first)};
	return (result);
}

inline str8
str8_zero(void)
{
	str8 result = {0};
	return (result);
}

inline str8
str8_cstr(char *c)
{
	str8 result = {(u8 *)c, cstr8_len((u8 *)c)};
	return (result);
}

inline str8
str8_cstr_capped(void *cstr, void *cap)
{
	char *ptr = (char *)cstr;
	char *opl = (char *)cap;
	for(; ptr < opl && *ptr != 0; ptr += 1);
	u64 size    = (u64)(ptr - (char *)cstr);
	str8 result = {(u8 *)cstr, size};
	return (result);
}

inline bool32
str8_match(str8 a, str8 b, str_match_flags flags)
{
	bool32 result = 0;
	if(a.size == b.size || (flags & str_match_flag_right_side_sloppy)) {
		bool32 case_insensitive  = (flags & str_match_flag_case_insensitive);
		bool32 slash_insensitive = (flags & str_match_flag_slash_insensitive);
		u64 size                 = MIN(a.size, b.size);
		result                   = 1;

		for(u64 i = 0; i < size; i += 1) {
			u8 at = a.str[i];
			u8 bt = b.str[i];
			if(case_insensitive) {
				at = char_to_upper(at);
				bt = char_to_upper(bt);
			}
			if(slash_insensitive) {
				at = char_to_correct_slash(at);
				bt = char_to_correct_slash(bt);
			}
			if(at != bt) {
				result = 0;
				break;
			}
		}
	}
	return (result);
}

inline void
str8_cpy(str8 *src, str8 *dst)
{
	dst->size = src->size;
	memcpy(src->str, dst->str, src->size);
	dst->str[dst->size] = 0;
}

inline str8
str8_cpy_push(struct alloc alloc, str8 src)
{
	str8 dst;
	dst.size = src.size;
	dst.str  = alloc.allocf(alloc.ctx, src.size + 1 * sizeof(u8));
	memcpy(dst.str, src.str, src.size);
	dst.str[dst.size] = 0;
	return dst;
}

inline str8
str8_cat_push(struct alloc alloc, str8 s1, str8 s2)
{
	str8 str;
	str.size = s1.size + s2.size;
	str.str  = alloc.allocf(alloc.ctx, str.size + 1 * sizeof(u8));
	memcpy(str.str, s1.str, s1.size);
	memcpy(str.str + s1.size, s2.str, s2.size);
	str.str[str.size] = 0;
	return (str);
}

str8
str8_fmtv_push(struct alloc alloc, char *fmt, va_list args)
{
	va_list args2;
	va_copy(args2, args);
	u32 needed_bytes        = stbsp_vsnprintf(0, 0, fmt, args) + 1;
	str8 result             = {0};
	result.str              = alloc.allocf(alloc.ctx, needed_bytes * sizeof(u8));
	result.size             = stbsp_vsnprintf((char *)result.str, needed_bytes, fmt, args2);
	result.str[result.size] = 0;
	va_end(args2);
	return (result);
}

str8
str8_fmt_push(struct alloc alloc, char *fmt, ...)
{
	assert(alloc.allocf != NULL);
	va_list args;
	va_start(args, fmt);
	str8 result = str8_fmtv_push(alloc, fmt, args);
	va_end(args);
	return (result);
}

inline f32
str8_to_f32(str8 str)
{
	f32 num   = 0.0;
	f32 mul   = 1.0;
	i32 dec   = 0;
	usize len = str.size;

	for(usize i = 0; i < len; i++) {
		if(str.str[i] == '.') dec = 1;
	}

	for(int idx = len - 1; idx >= 0; idx--) {
		if(str.str[idx] == '-')
			num = -num;
		else if(str.str[idx] == '.')
			dec = 0;
		else if(dec) {
			num += str.str[idx] - '0';
			num *= 0.1f;
		} else {
			num += (str.str[idx] - '0') * mul;
			mul *= 10.0f;
		}
	}

	return num;
}
