#pragma once

#include "base/utils.h"
#include "lib/fnt/fnt-defs.h"
#include "lib/serialize/serialize.h"

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

i32 fnt_char_size_x_px(struct fnt fnt, i32 a, i32 b, i32 tracking);
v2_i32 fnt_size_px(struct fnt fnt, const str8 str, i32 tracking, i32 leading);
i32 fnt_mono_size_x_px(struct fnt fnt, str8 str, i32 tracking);

struct fnt_ascii *fnt_ascii_ini(struct alloc alloc, struct fnt *fnt);
b32 fnt_ascii_has_glyphs(struct fnt fnt);
void fnt_write(struct fnt fnt, struct ser_writer *w);
i32 fnt_read(struct alloc alloc, struct ser_reader *r, struct fnt *fnt);
struct fnt fnt_load_from_mem(struct alloc alloc, void *data, usize size);
struct fnt fnt_load(str8 path, struct alloc alloc, struct alloc scratch);
struct str8_list fnt_wrap_lines_from_str(struct alloc alloc, struct fnt fnt, str8 str, i32 max_width, i32 tracking, i32 leading);
