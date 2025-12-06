#include "tex.h"
#include "base/dbg.h"
#include "base/utils.h"
#include "engine/gfx/gfx-defs.h"
#include "sys/sys-intrin.h"
#include "sys/sys-io.h"
#include "base/log.h"
#include "base/trace.h"

// mask is almost always = 1
struct tex
tex_create_internal(i32 w, i32 h, b32 mask, struct alloc alloc)
{
	struct tex t = {0};

	// NOTICE: Seems that the tex should be padded on creation
	// If not the correct size won't be calculated
	// Align the next multiple of 32 greater than or equal to the
	i32 width_alinged = (w + 31) & ~31;

	// Calculates the number of words needed for the width of the texture
	// It multiplies by 2 if it uses a mask (transparency)
	// by shifting it by 1 << (0 < mask)
	i32 width_word = (width_alinged / 32) << (0 < mask);

	// So each `word` is a row of pixels aligned
	// To get the size we multiply by the height
	// getting the full size of the image aligned.
	usize size = sizeof(u32) * width_word * h * 2;

	void *mem = alloc.allocf(alloc.ctx, size, 4);

	if(!mem) return t;

	t.px    = (u32 *)mem;
	t.fmt   = (0 < mask);
	t.w     = w;
	t.h     = h;
	t.wword = width_word;
	return t;
}

struct tex
tex_create(i32 w, i32 h, struct alloc alloc)
{
	return tex_create_internal(w, h, 1, alloc);
}

struct tex
tex_create_opaque(i32 w, i32 h, struct alloc alloc)
{
	return tex_create_internal(w, h, 0, alloc);
}

struct tex
tex_load(str8 path, struct alloc alloc)
{
	struct tex res = {0};
	void *f        = sys_file_open_r(path);
	dbg_check(f, "tex", "failed to open texture %s", path.str);

	struct tex_header header = {0};
	dbg_check(sys_file_r(f, &header, sizeof(struct tex_header)), "tex", "failed to read tex header %s", path.str);

	i32 width_alinged = (header.w + 31) & ~31;
	usize tex_size    = ((width_alinged * header.h) * 2) / 8;
	struct tex t      = tex_create(header.w, header.h, alloc);
	sys_file_r(f, t.px, tex_size);

error:;
	if(f) { sys_file_close(f); }
	return t;
}

void
tex_clr(struct tex dst, i32 col)
{
	TRACE_START(__func__);

	i32 nn = dst.wword * dst.h;
	u32 *p = dst.px;
	if(!p) return;
	switch(col) {
	case GFX_COL_BLACK:
		switch(dst.fmt) {
		case TEX_FMT_OPAQUE:
			for(i32 n = 0; n < nn; n++) {
				*p++ = 0U;
			}
			break;
		case TEX_FMT_MASK:
			for(i32 n = 0; n < nn; n += 2) {
				*p++ = 0U;          // data
				*p++ = 0xFFFFFFFFU; // mask
			}
			break;
		}
		break;
	case GFX_COL_WHITE:
		switch(dst.fmt) {
		case TEX_FMT_OPAQUE:
			for(i32 n = 0; n < nn; n++) {
				*p++ = 0xFFFFFFFFU;
			}
			break;
		case TEX_FMT_MASK:
			for(i32 n = 0; n < nn; n += 2) {
				*p++ = 0xFFFFFFFFU; // data
				*p++ = 0xFFFFFFFFU; // mask
			}
			break;
		}
		break;
	case GFX_COL_CLEAR:
		if(dst.fmt == TEX_FMT_OPAQUE) break;
		for(i32 n = 0; n < nn; n++) {
			*p++ = 0;
		}
		break;
	}
	TRACE_END();
}

static i32
tex_px_at_unsafe(struct tex tex, i32 x, i32 y)
{
	u32 b = bswap_u32(0x80000000U >> (x & 31));
	switch(tex.fmt) {
	case TEX_FMT_MASK: return (tex.px[y * tex.wword + ((x >> 5) << 1)] & b);
	case TEX_FMT_OPAQUE: return (tex.px[y * tex.wword + (x >> 5)] & b);
	}
	return 0;
}

static i32
tex_mask_at_unsafe(struct tex tex, i32 x, i32 y)
{
	if(tex.fmt == TEX_FMT_OPAQUE) return 1;

	u32 b = bswap_u32(0x80000000U >> (x & 31));
	return (tex.px[y * tex.wword + ((x >> 5) << 1) + 1] & b);
}

static void
tex_px_unsafe(struct tex tex, i32 x, i32 y, i32 col)
{
	u32 b  = bswap_u32(0x80000000U >> (x & 31));
	u32 *p = NULL;
	switch(tex.fmt) {
	case TEX_FMT_MASK: p = &tex.px[y * tex.wword + ((x >> 5) << 1)]; break;
	case TEX_FMT_OPAQUE: p = &tex.px[y * tex.wword + (x >> 5)]; break;
	default: return;
	}
	*p = (col == 0 ? *p & ~b : *p | b);
}

void
tex_px_unsafe_display(struct tex tex, i32 x, i32 y, i32 col)
{
	u32 b  = bswap_u32(0x80000000U >> (x & 31));
	u32 *p = &tex.px[y * tex.wword + (x >> 5)];
	*p     = (col == 0 ? *p & ~b : *p | b);
}

static void
tex_mask_unsafe(struct tex tex, i32 x, i32 y, i32 col)
{
	if(tex.fmt == TEX_FMT_OPAQUE) return;
	u32 b  = bswap_u32(0x80000000U >> (x & 31));
	u32 *p = &tex.px[y * tex.wword + ((x >> 5) << 1) + 1];
	*p     = (col == 0 ? *p & ~b : *p | b);
}

i32
tex_px_at(struct tex tex, i32 x, i32 y)
{
	if(!(0 <= x && x < tex.w && 0 <= y && y < tex.h)) return 0;
	return tex_px_at_unsafe(tex, x, y);
}

i32
tex_mask_at(struct tex tex, i32 x, i32 y)
{
	if(!(0 <= x && x < tex.w && 0 <= y && y < tex.h)) return 1;
	return tex_mask_at_unsafe(tex, x, y);
}

void
tex_px(struct tex tex, i32 x, i32 y, i32 col)
{
	if(0 <= x && x < tex.w && 0 <= y && y < tex.h) {
		tex_px_unsafe(tex, x, y, col);
	}
}

void
tex_mask(struct tex tex, i32 x, i32 y, i32 col)
{
	if(0 <= x && x < tex.w && 0 <= y && y < tex.h) {
		tex_mask_unsafe(tex, x, y, col);
	}
}

void
tex_opaque_to_rgba(struct tex tex, u32 *out, ssize size, struct gfx_col_pallete pallete)
{
	dbg_assert(tex.fmt == TEX_FMT_OPAQUE);
	u32 *pixels       = out;
	i32 width_alinged = (tex.w + 31) & ~31;
	i32 wbytes        = width_alinged / 8;
	u8 *in            = (u8 *)tex.px;
	dbg_assert(size >= tex.w * tex.h);
	for(i32 y = 0; y < tex.h; y++) {
		for(i32 x = 0; x < tex.w; x++) {
			i32 src        = (x >> 3) + y * wbytes;
			i32 dst        = x + y * tex.w;
			i32 byt        = in[src];
			u8 mask        = 0x80 >> (x & 7);
			i32 bit        = !!(byt & mask);
			u32 color_bgra = bit ? pallete.colors[GFX_COL_WHITE] : pallete.colors[GFX_COL_BLACK];
			u32 color_rgba = bswap_u32(color_bgra);
			pixels[dst]    = color_rgba;
		}
	}
}

void
tex_opaque_to_pdi(struct tex tex, u8 *out_px, i32 w, i32 h, i32 row_bytes)
{
	dbg_assert(tex.fmt == TEX_FMT_OPAQUE);
	i32 src_stride = tex.wword * 4;
	i32 y2         = MIN(h, tex.h);
	i32 copy_bytes = MIN(row_bytes, src_stride);
	const u8 *src  = (const u8 *)tex.px;

	for(i32 y = 0; y < y2; ++y)
		mcpy(out_px + y * row_bytes,
			src + y * src_stride,
			copy_bytes);
}

void
tex_mask_to_pdi(struct tex tex, u8 *px_out, u8 *mask_out, i32 w, i32 h, i32 row_bytes)
{
	dbg_assert(tex.fmt == TEX_FMT_MASK);
	i32 w_aligned  = (tex.w + 31) & ~31;
	i32 wbyte      = tex.wword * 4;
	u32 *color_dst = (u32 *)px_out;
	u32 *mask_dst  = (u32 *)mask_out;
	const u32 *src = (const u32 *)tex.px;
	i32 y2         = MIN(h, tex.h);
	i32 x2         = MIN(row_bytes, wbyte) / 4;
	i32 stride     = row_bytes / 4;

	for(i32 y = 0; y < y2; ++y) {
		for(i32 x = 0; x < x2; ++x) {
			u32 color_word            = *src++;
			u32 mask_word             = *src++;
			color_dst[y * stride + x] = color_word;
			mask_dst[y * stride + x]  = mask_word;
		}
	}
}

void
tex_cpy(struct tex *dst, struct tex *src)
{
	// TODO: For now only can copy textures that are the same dimensions
	dbg_assert(src->fmt == dst->fmt);
	dbg_assert(src->w == dst->w);
	dbg_assert(src->h == dst->h);
	dbg_assert(src->wword == dst->wword);
	dbg_assert(src->px != NULL);
	dbg_assert(dst->px != NULL);
	usize mem_size = sizeof(u32) * dst->wword * dst->h * 2;
	mcpy(dst->px, src->px, mem_size);
}

b32
tex_from_rgba_w(const struct pixel_u8 *data, i32 w, i32 h, str8 out_path)
{
	b32 res    = false;
	void *file = sys_file_open_w(out_path);
	dbg_check(file, "tex", "failed to open file to write: %s", out_path.str);
	struct tex_header header = {.w = w, .h = h};
	dbg_check(sys_file_w(file, &header, sizeof(header)) == 1, "tex", "Error writing header image to file");

	i32 w_aligned = (w + 31) & ~31;
	ssize bit_idx = 0;
	u32 color_row = 0;
	u32 mask_row  = 0;
	for(ssize y = 0; y < h; ++y) {
		const struct pixel_u8 *row = (struct pixel_u8 *)(data + y * w);
		for(i32 x = 0; x < w_aligned; ++x) {
			struct pixel_u8 pixel = {0};
			if(x < w) {
				pixel = row[x];
			}

			i32 color      = (pixel.r + pixel.g + pixel.b) > (3 * 127);
			i32 alpha      = pixel.a > 127;
			u32 pixel_mask = (1u << (31 - bit_idx));

			if(color) { color_row |= pixel_mask; }
			if(alpha) { mask_row |= pixel_mask; }

			if(++bit_idx == 32) {
				color_row = bswap_u32(color_row);
				mask_row  = bswap_u32(mask_row);
				dbg_check(sys_file_w(file, &color_row, sizeof(u32)), "tex", "failed to write color data to file");
				dbg_check(sys_file_w(file, &mask_row, sizeof(u32)), "tex", "failed to write mask data to file");
				color_row = 0;
				mask_row  = 0;
				bit_idx   = 0;
			}
		}
		// flush remainder bits if width not multiple of 32
		if(bit_idx > 0) {
			color_row = bswap_u32(color_row);
			mask_row  = bswap_u32(mask_row);
			dbg_check(sys_file_w(file, &color_row, sizeof(u32)) == 1,
				"tex",
				"failed to write color data (partial)");
			dbg_check(sys_file_w(file, &mask_row, sizeof(u32)) == 1,
				"tex",
				"failed to write mask data (partial)");
			color_row = mask_row = 0;
			bit_idx              = 0;
		}
	}

	res = true;
error:;
	if(file) { sys_file_close(file); }
	return res;
}
