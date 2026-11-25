#pragma once

#include "base/str.h"
#include "base/utils.h"
#include "lib/fnt/fnt-defs.h"
#include "lib/serialize/serialize.h"

static inline b32
fnt_code_point_is_variation_selector(u32 code_point)
{
	return code_point == 0xFE0E || code_point == 0xFE0F;
}

static inline b32
fnt_code_point_is_cjk(u32 cp)
{
	return (cp >= 0x2E80 && cp <= 0x2FDF) ||  // radicals
		(cp >= 0x3000 && cp <= 0x30FF) ||     // CJK symbols and punctuation, kana
		(cp >= 0x3100 && cp <= 0x312F) ||     // bopomofo
		(cp >= 0x31F0 && cp <= 0x31FF) ||     // katakana extensions
		(cp >= 0x3400 && cp <= 0x4DBF) ||     // ext A
		(cp >= 0x4E00 && cp <= 0x9FFF) ||     // unified ideographs
		(cp >= 0xF900 && cp <= 0xFAFF) ||     // compatibility ideographs
		(cp >= 0xFE30 && cp <= 0xFE4F) ||     // compatibility forms
		(cp >= 0xFF00 && cp <= 0xFF60) ||     // fullwidth forms
		(cp >= 0x20000 && cp <= 0x3FFFF);     // ext B and up
}

static inline b32
fnt_code_point_breaks_as_cjk(u32 cp)
{
	switch(cp) {
	case 0x00B7: // ·
	case 0x2014: // —
	case 0x2018: // ‘
	case 0x2019: // ’
	case 0x201C: // “
	case 0x201D: // ”
	case 0x2025: // ‥
	case 0x2026: // …
		return true;
	default:
		return fnt_code_point_is_cjk(cp);
	}
}

static inline b32
fnt_code_point_no_line_start(u32 cp)
{
	switch(cp) {
	case 0x3001: // 、
	case 0x3002: // 。
	case 0xFF0C: // ，
	case 0xFF0E: // ．
	case 0xFF1A: // ：
	case 0xFF1B: // ；
	case 0xFF1F: // ？
	case 0xFF01: // ！
	case 0xFF09: // ）
	case 0xFF3D: // ］
	case 0xFF5D: // ｝
	case 0xFF05: // ％
	case 0xFF5E: // ～
	case 0x3009: // 〉
	case 0x300B: // 》
	case 0x300D: // 」
	case 0x300F: // 』
	case 0x3011: // 】
	case 0x3015: // 〕
	case 0x3017: // 〗
	case 0x3019: // 〙
	case 0x301B: // 〛
	case 0x301E: // 〞
	case 0x301F: // 〟
	case 0x3005: // 々
	case 0x309D: // ゝ
	case 0x309E: // ゞ
	case 0x30FB: // ・
	case 0x30FC: // ー
	case 0x2019: // ’
	case 0x201D: // ”
	case 0x2026: // …
	case 0x2025: // ‥
	case 0x2014: // —
	case 0x00B7: // ·
		return true;
	default:
		return false;
	}
}

static inline b32
fnt_code_point_no_line_end(u32 cp)
{
	switch(cp) {
	case 0xFF08: // （
	case 0xFF3B: // ［
	case 0xFF5B: // ｛
	case 0x3008: // 〈
	case 0x300A: // 《
	case 0x300C: // 「
	case 0x300E: // 『
	case 0x3010: // 【
	case 0x3014: // 〔
	case 0x3016: // 〖
	case 0x3018: // 〘
	case 0x301A: // 〚
	case 0x301D: // 〝
	case 0x2018: // ‘
	case 0x201C: // “
	case 0xFF04: // ＄
	case 0xFFE5: // ￥
		return true;
	default:
		return false;
	}
}

// Returns the code point at *i and moves *i past it and any variation selectors after it.
// U32_MAX at the end of str, FNT_CODE_POINT_MISSING for a malformed byte.
static inline u32
fnt_code_point_next(str8 str, usize *i)
{
	if(*i >= str.size) { return U32_MAX; }

	struct unicode_decode decode = utf8_decode(str.str + *i, str.size - *i);
	u32 res                      = decode.codepoint == U32_MAX ? FNT_CODE_POINT_MISSING : decode.codepoint;
	*i += decode.inc;

	while(*i < str.size) {
		struct unicode_decode selector = utf8_decode(str.str + *i, str.size - *i);
		if(!fnt_code_point_is_variation_selector(selector.codepoint)) { break; }
		*i += selector.inc;
	}

	return res;
}

static inline b32
fnt_ascii_has(struct fnt fnt, u32 code_point)
{
	return fnt.ascii && code_point < FNT_ASCII_MAX && ((fnt.ascii->mask[code_point / 64] >> (code_point % 64)) & 1);
}

static inline i32
fnt_utf8_idx(struct fnt_utf8 *utf8, u32 code_point)
{
	i32 lo = 0;
	i32 hi = (i32)utf8->count - 1;
	while(lo <= hi) {
		i32 mid = (lo + hi) / 2;
		u32 v   = utf8->code_points[mid];
		if(v == code_point) { return mid; }
		if(v < code_point) {
			lo = mid + 1;
		} else {
			hi = mid - 1;
		}
	}
	return -1;
}

static inline struct fnt_glyph
fnt_glyph(struct fnt fnt, u32 code_point)
{
	struct fnt_glyph res = {.cell = -1, .w = 0};
	u32 lookup[]         = {code_point, FNT_CODE_POINT_MISSING, '?'};

	for(usize i = 0; i < ARRLEN(lookup); ++i) {
		u32 c = lookup[i];
		if(fnt_ascii_has(fnt, c)) {
			res.cell = fnt.ascii->cells[c];
			res.w    = fnt.ascii->widths[c];
			break;
		}
		i32 idx = fnt.utf8 ? fnt_utf8_idx(fnt.utf8, c) : -1;
		if(idx >= 0) {
			res.cell = fnt.utf8->cells[idx];
			res.w    = fnt.utf8->widths[idx];
			break;
		}
	}

	if(res.w == 0) { res.w = fnt.cell_w; }
	return res;
}

static inline i32
fnt_ascii_kern_idx(u32 a, u32 b)
{
	return (a - FNT_ASCII_KERN_MIN) * FNT_ASCII_KERN_MAX + (b - FNT_ASCII_KERN_MIN);
}

static inline i32
fnt_kern(struct fnt fnt, u32 a, u32 b)
{
	if(
		a >= FNT_ASCII_KERN_MIN &&
		a < FNT_ASCII_MAX &&
		b >= FNT_ASCII_KERN_MIN &&
		b < FNT_ASCII_MAX) {
		if(fnt.ascii == NULL || fnt.ascii->kern == NULL) { return 0; }
		return fnt.ascii->kern[fnt_ascii_kern_idx(a, b)];
	}

	if(fnt.utf8 == NULL) { return 0; }

	u64 key = ((u64)a << 32) | b;
	i32 lo  = 0;
	i32 hi  = (i32)fnt.utf8->kern_count - 1;
	while(lo <= hi) {
		i32 mid = (lo + hi) / 2;
		u64 v   = fnt.utf8->kern_keys[mid];
		if(v == key) { return fnt.utf8->kern_vals[mid]; }
		if(v < key) {
			lo = mid + 1;
		} else {
			hi = mid - 1;
		}
	}
	return 0;
}

static inline i32
fnt_glyph_cell(struct fnt fnt, u32 code_point)
{
	return fnt_glyph(fnt, code_point).cell;
}

i32 fnt_char_size_x_px(struct fnt fnt, u32 a, u32 b, i32 tracking);
v2_i32 fnt_size_px(struct fnt fnt, const str8 str, i32 tracking, i32 leading);
i32 fnt_mono_size_x_px(struct fnt fnt, str8 str, i32 tracking);

struct fnt_ascii *fnt_ascii_ini(struct alloc alloc, struct fnt *fnt);
b32 fnt_ascii_has_glyphs(struct fnt fnt);
struct fnt_utf8 *fnt_utf8_ini(struct alloc alloc, struct fnt *fnt);
void fnt_write(struct fnt fnt, struct ser_writer *w);
i32 fnt_read(struct alloc alloc, struct ser_reader *r, struct fnt *fnt);
struct fnt fnt_load_from_mem(struct alloc alloc, void *data, usize size);
struct fnt fnt_load(str8 path, struct alloc alloc, struct alloc scratch);
struct str8_list fnt_wrap_lines_from_str(struct alloc alloc, struct fnt fnt, str8 str, i32 max_width, i32 tracking, i32 leading);
