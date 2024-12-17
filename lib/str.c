#include "str.h"
#include "mathfunc.h"
#include "sys-str.h"
#include "sys-utils.h"
#include "sys-assert.h"

// NOTE: linked list macro helpers
// TODO: Move this, undestand it
#define CheckNil(nil, p)                    ((p) == 0 || (p) == nil)
#define SetNil(nil, p)                      ((p) = nil)
#define SLLQueuePush_NZ(nil, f, l, n, next) (CheckNil(nil, f) ? ((f) = (l) = (n), SetNil(nil, (n)->next)) : ((l)->next = (n), (l) = (n), SetNil(nil, (n)->next)))
#define SLLQueuePush(f, l, n)               SLLQueuePush_NZ(0, f, l, n, next)

u8 INTEGER_SYMBOLS[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

u8 INTEGER_SYMBOL_REVERSE[128] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

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

inline bool32
char_is_ascii(u8 c)
{
	if((c & 0x80) == 0x00) return true;
	return false;
}

inline bool32
char_is_utf8(u8 c)
{
	if((c & 0x80) == 0x00) {
		return true; // 1-byte ASCII (0xxxxxxx)
	} else if((c & 0xE0) == 0xC0) {
		return true; // 2-byte UTF-8 (110xxxxx)
	} else if((c & 0xF0) == 0xE0) {
		return true; // 3-byte UTF-8 (1110xxxx)
	} else if((c & 0xF8) == 0xF0) {
		return true; // 4-byte UTF-8 (11110xxx)
	} else {
		return false;
	}
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

inline usize
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
	mcpy(dst->str, src->str, src->size);
	dst->str[dst->size] = 0;
}

inline str8
str8_cpy_push(struct alloc alloc, str8 src)
{
	str8 dst;
	dst.size = src.size;
	dst.str  = alloc.allocf(alloc.ctx, src.size + 1 * sizeof(u8));
	mcpy(dst.str, src.str, src.size);
	dst.str[dst.size] = 0;
	return dst;
}

inline str8
str8_cat_push(struct alloc alloc, str8 s1, str8 s2)
{
	str8 str;
	str.size = s1.size + s2.size;
	str.str  = alloc.allocf(alloc.ctx, str.size + 1 * sizeof(u8));
	mcpy(str.str, s1.str, s1.size);
	mcpy(str.str + s1.size, s2.str, s2.size);
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

str8
str8_substr(str8 str, union rng_u64 range)
{
	range.min = MIN(range.min, str.size);
	range.max = MIN(range.max, str.size);
	str.str += range.min;
	str.size = range.max - range.min;
	return (str);
}

str8
str8_prefix(str8 str, usize size)
{
	str.size = MIN(size, str.size);
	return (str);
}

str8
str8_skip(str8 str, usize amt)
{
	amt = MIN(amt, str.size);
	str.str += amt;
	str.size -= amt;
	return (str);
}

str8
str8_postfix(str8 str, usize size)
{
	size     = MIN(size, str.size);
	str.str  = (str.str + str.size) - size;
	str.size = size;
	return (str);
}

str8
str8_chop(str8 str, usize amt)
{
	amt = MIN(amt, str.size);
	str.size -= amt;
	return (str);
}

str8
str8_skip_chop_whitespace(str8 str)
{
	u8 *first = str.str;
	u8 *opl   = first + str.size;
	for(; first < opl; first += 1) {
		if(!char_is_space(*first)) {
			break;
		}
	}
	for(; opl > first;) {
		opl -= 1;
		if(!char_is_space(*opl)) {
			opl += 1;
			break;
		}
	}
	str8 result = str8_range(first, opl);
	return (result);
}

bool32
str8_ends_with(str8 str, str8 end, str_match_flags flags)
{
	str8 postfix    = str8_postfix(str, end.size);
	bool32 is_match = str8_match(end, postfix, flags);
	return is_match;
}

bool32
str8_starts_with(str8 str, str8 start, str_match_flags flags)
{
	str_match_flags adjusted_flags = flags | str_match_flag_right_side_sloppy;
	bool32 is_match                = str8_match(str, start, adjusted_flags);
	return is_match;
}

usize
str8_find_needle(str8 str, usize start_pos, str8 needle, str_match_flags flags)
{
	u8 *p             = str.str + start_pos;
	usize stop_offset = MAX(str.size + 1, needle.size) - needle.size;
	u8 *stop_p        = str.str + stop_offset;

	if(needle.size > 0) {
		u8 *string_opl                 = str.str + str.size;
		str8 needle_tail               = str8_skip(needle, 1);
		str_match_flags adjusted_flags = flags | str_match_flag_right_side_sloppy;
		u8 needle_first_char_adjusted  = needle.str[0];
		if(adjusted_flags & str_match_flag_case_insensitive) {
			needle_first_char_adjusted = char_to_upper(needle_first_char_adjusted);
		}
		for(; p < stop_p; p += 1) {
			u8 haystack_char_adjusted = *p;
			if(adjusted_flags & str_match_flag_case_insensitive) {
				haystack_char_adjusted = char_to_upper(haystack_char_adjusted);
			}
			if(haystack_char_adjusted == needle_first_char_adjusted) {
				if(str8_match(str8_range(p + 1, string_opl), needle_tail, adjusted_flags)) {
					break;
				}
			}
		}
	}

	usize result = str.size;
	if(p < stop_p) {
		result = (usize)(p - str.str);
	}
	return (result);
}

union rng_u64
rng_1u64(u64 min, u64 max)
{
	union rng_u64 r = {{min, max}};
	if(r.min > r.max) { SWAP(u64, r.min, r.max); }
	return r;
}

usize
str8_find_needle_reverse(
	str8 str,
	usize start_pos,
	str8 needle,
	str_match_flags flags)
{
	u64 result = 0;
	for(i64 i = str.size - start_pos - needle.size; i >= 0; --i) {
		str8 haystack = str8_substr(str, rng_1u64(i, i + needle.size));
		if(str8_match(haystack, needle, flags)) {
			result = (u64)i + needle.size;
			break;
		}
	}
	return result;
}

i32
str8_to_i32(str8 str)
{
	i32 res = 0;
	sys_parse_string((char *)str.str, "%d", &res);
	return res;
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

str8
str8_chop_last_slash(str8 str)
{
	if(str.size > 0) {
		u8 *ptr = str.str + str.size - 1;
		for(; ptr >= str.str; ptr -= 1) {
			if(*ptr == '/' || *ptr == '\\') {
				break;
			}
		}
		if(ptr >= str.str) {
			str.size = (usize)(ptr - str.str);
		} else {
			str.size = 0;
		}
	}
	return (str);
}

str8
str8_skip_last_slash(str8 str)
{
	if(str.size > 0) {
		u8 *ptr = str.str + str.size - 1;
		for(; ptr >= str.str; ptr -= 1) {
			if(*ptr == '/' || *ptr == '\\') {
				break;
			}
		}
		if(ptr >= str.str) {
			ptr += 1;
			str.size = (usize)(str.str + str.size - ptr);
			str.str  = ptr;
		}
	}
	return (str);
}

str8
str8_chop_last_dot(str8 str)
{
	str8 result = str;
	usize p     = str.size;
	for(; p > 0;) {
		p -= 1;
		if(str.str[p] == '.') {
			result = str8_prefix(str, p);
			break;
		}
	}
	return (result);
}

str8
str8_skip_last_dot(str8 str)
{
	str8 result = str;
	usize p     = str.size;
	for(; p > 0;) {
		p -= 1;
		if(str.str[p] == '.') {
			result = str8_skip(str, p + 1);
			break;
		}
	}
	return (result);
}

struct str8_node *
str8_list_push_node_set_string(struct str8_list *list, struct str8_node *node, str8 str)
{
	SLLQueuePush(list->first, list->last, node);
	list->node_count += 1;
	list->total_size += str.size;
	node->str = str;
	return (node);
}

struct str8_node *
str8_list_push(struct alloc alloc, struct str8_list *list, str8 str)
{
	struct str8_node *node = alloc.allocf(alloc.ctx, sizeof(struct str8_node));
	str8_list_push_node_set_string(list, node, str);
	return (node);
}

struct str8_node *
str8_list_pushf(struct alloc alloc, struct str8_list *list, char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	str8 str                 = str8_fmtv_push(alloc, fmt, args);
	struct str8_node *result = str8_list_push(alloc, list, str);
	va_end(args);
	return (result);
}

struct str8_list
str8_split(struct alloc alloc, str8 str, u8 *split_chars, usize split_char_count, str_split_flags flags)
{
	struct str8_list list = {0};
	bool32 keep_empties   = (flags & str_split_flag_keep_empties);

	u8 *ptr = str.str;
	u8 *opl = str.str + str.size;
	for(; ptr < opl;) {
		u8 *first = ptr;
		for(; ptr < opl; ptr += 1) {
			u8 c            = *ptr;
			bool32 is_split = 0;
			for(usize i = 0; i < split_char_count; i += 1) {
				if(split_chars[i] == c) {
					is_split = 1;
					break;
				}
			}
			if(is_split) {
				break;
			}
		}

		str8 string = str8_range(first, ptr);
		if(keep_empties || string.size > 0) {
			str8_list_push(alloc, &list, string);
		}
		ptr += 1;
	}

	return list;
}

struct str8_list
str8_split_by_string_chars(struct alloc alloc, str8 str, str8 split_chars, str_split_flags flags)
{
	struct str8_list list = str8_split(alloc, str, split_chars.str, split_chars.size, flags);
	return list;
}

struct str8_list
str8_split_path(struct alloc alloc, str8 str)
{
	struct str8_list res = str8_split(alloc, str, (u8 *)"/\\", 2, 0);
	return res;
}

struct str8_list
wrapped_lines_from_str(
	struct alloc alloc,
	str8 str,
	usize first_line_max_width,
	usize max_width,
	usize wrap_indent)
{
	struct str8_list list      = {0};
	union rng_u64 line_range   = rng_u64(0, 0);
	usize wrapped_indent_level = 0;
	static char *spaces        = "                                                                ";
	for(usize idx = 0; idx <= str.size; idx += 1) {
		u8 chr = idx < str.size ? str.str[idx] : 0;
		if(chr == '\n') {
			union rng_u64 candidate_line_range = line_range;
			candidate_line_range.max           = idx;
			// NOTE: when wrapping is interrupted with \n we emit a string without including \n
			// because later tool_fprint_list inserts separator after each node
			// except for last node, so don't strip last \n.
			if(idx + 1 == str.size) {
				candidate_line_range.max += 1;
			}
			str8 substr = str8_substr(str, candidate_line_range);
			str8_list_push(alloc, &list, substr);
			line_range = rng_u64(idx + 1, idx + 1);
		} else if(char_is_space(chr) || chr == 0) {
			union rng_u64 candidate_line_range = line_range;
			candidate_line_range.max           = idx;
			str8 substr                        = str8_substr(str, candidate_line_range);
			usize width_this_line              = max_width - wrapped_indent_level;
			if(list.node_count == 0) {
				width_this_line = first_line_max_width;
			}
			if(substr.size > width_this_line) {
				str8 line = str8_substr(str, line_range);
				if(wrapped_indent_level > 0) {
					line = str8_fmt_push(alloc, "%.*s%.*s", wrapped_indent_level, spaces, line.size, line.str);
				}
				str8_list_push(alloc, &list, line);
				line_range           = rng_u64(line_range.max + 1, candidate_line_range.max);
				wrapped_indent_level = MIN(64, wrap_indent);
			} else {
				line_range = candidate_line_range;
			}
		}
	}
	if(line_range.min < str.size && line_range.max > line_range.min) {
		str8 line = str8_substr(str, line_range);
		if(wrapped_indent_level > 0) {
			line = str8_fmt_push(alloc, "%.*s%.*s", wrapped_indent_level, spaces, line.size, line.str);
		}
		str8_list_push(alloc, &list, line);
	}
	return list;
}
