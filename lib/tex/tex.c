#include "tex.h"
#include "base/dbg.h"
#include "base/types.h"
#include "base/utils.h"
#include "engine/gfx/gfx-defs.h"
#include "base/intrin.h"
#include "sys/sys-io.h"
#include "sys/sys-lz4.h"
#include "sys/sys.h"

// Words per row: width padded to 32px; mask fmt doubles for alpha
static inline u32
tex_wword(i32 w, b32 mask)
{
	u32 waligned = (w + 31) & ~31;
	return (waligned >> 5) << (mask != 0);
}

// Pixel buffer bytes for w x h
static inline ssize
tex_mem_size(i32 w, i32 h, b32 mask)
{
	return (ssize)sizeof(u32) * tex_wword(w, mask) * (ssize)h;
}

struct tex
tex_create_internal(struct alloc alloc, i32 w, i32 h, b32 mask)
{
	struct tex res = {0};
	b32 m          = mask != 0;
	u32 wword      = tex_wword(w, m);
	ssize size     = tex_mem_size(w, h, m);
	void *mem      = alloc_size_aligned(alloc, size, MEM_ALIGN_PD_CACHE, false);
	if(mem) {
		res.px    = (u32 *)mem;
		res.fmt   = m;
		res.w     = w;
		res.h     = h;
		res.wword = wword;
	}
	return res;
}

struct tex
tex_create(struct alloc alloc, i32 w, i32 h)
{
	return tex_create_internal(alloc, w, h, 1);
}

struct tex
tex_create_opaque(struct alloc alloc, i32 w, i32 h)
{
	return tex_create_internal(alloc, w, h, 0);
}

struct tex
tex_load(struct alloc alloc, struct alloc scratch, str8 path)
{
	struct tex res           = {0};
	sys_file f               = sys_file_open_r(path);
	struct tex_header header = {0};
	struct tex t             = {0};
	ssize tex_size           = 0;
	ssize file_size          = 0;
	ssize packed_len         = 0;
	u8 *packed               = NULL;
	i32 n                    = 0;

	dbg_check(sys_file_is_valid(f), "tex", "failed to open texture %s", path.str);
	dbg_check(sys_file_r(f, &header, sizeof(struct tex_header)) == (ssize)sizeof(struct tex_header), "tex", "failed to read tex header %s", path.str);
	dbg_check(header.fmt == TEX_FMT_OPAQUE || header.fmt == TEX_FMT_MASK,
		"tex",
		"invalid tex fmt %u",
		header.fmt);
	dbg_check(header.w > 0 && header.h > 0,
		"tex",
		"invalid tex size %ux%u",
		header.w,
		header.h);
	dbg_check((header.flags & ~TEX_FLAG_LZ4) == 0,
		"tex",
		"invalid tex flags %u",
		header.flags);

	t        = tex_create_internal(alloc, header.w, header.h, header.fmt);
	tex_size = tex_mem_size(header.w, header.h, header.fmt);
	dbg_check(t.px, "tex", "tex alloc failed %s", path.str);

	if(header.flags & TEX_FLAG_LZ4) {
		sys_file_seek_end(f, 0);
		file_size = sys_file_tell(f);
		dbg_check(file_size > (ssize)sizeof(header), "tex", "tex lz4 empty %s", path.str);
		packed_len = file_size - (ssize)sizeof(header);
		sys_file_seek_set(f, (ssize)sizeof(header));

		packed = mem_alloc_size(scratch, (usize)packed_len);
		dbg_check(packed, "tex", "tex lz4 staging failed %s", path.str);
		dbg_check(sys_file_r(f, packed, (u32)packed_len) == packed_len, "tex", "tex lz4 read failed %s", path.str);

		n = sys_lz4_decompress(packed, t.px, packed_len, tex_size);
		dbg_check(n == (int)tex_size, "tex", "tex lz4 decode failed %s", path.str);
	} else {
		dbg_check(sys_file_r(f, t.px, (u32)tex_size) == tex_size, "tex", "tex pixels read failed %s", path.str);
	}

	res = t;

error:;
	if(sys_file_is_valid(f)) { sys_file_close(f); }
	return res;
}

struct tex
tex_load_from_mem(struct alloc alloc, void *data, ssize size)
{
	struct tex res           = {0};
	u8 *src                  = data;
	struct tex_header header = {0};
	ssize tex_size           = 0;
	ssize packed_len         = 0;
	i32 n                    = 0;

	dbg_check(data, "tex", "null tex data");
	dbg_check(size >= (ssize)sizeof(header), "tex", "tex blob too small");

	mcpy(&header, src, sizeof(header));

	dbg_check(header.fmt == TEX_FMT_OPAQUE || header.fmt == TEX_FMT_MASK,
		"tex",
		"invalid tex fmt %u",
		header.fmt);
	dbg_check(header.w > 0 && header.h > 0,
		"tex",
		"invalid tex size %ux%u",
		header.w,
		header.h);
	dbg_check((header.flags & ~TEX_FLAG_LZ4) == 0,
		"tex",
		"invalid tex flags %u",
		header.flags);

	tex_size = tex_mem_size(header.w, header.h, header.fmt);
	res      = tex_create_internal(alloc, header.w, header.h, header.fmt);
	dbg_check_mem(res.px, "tex");

	if(header.flags & TEX_FLAG_LZ4) {
		packed_len = size - (ssize)sizeof(header);
		dbg_check(packed_len > 0, "tex", "tex lz4 empty");
		n = sys_lz4_decompress(src + sizeof(header), res.px, packed_len, tex_size);
		dbg_check(n == (int)tex_size, "tex", "tex lz4 decode failed %d %d", n, (i32)tex_size);
	} else {
		dbg_check(size >= (ssize)sizeof(header) + tex_size, "tex", "tex blob truncated");
		mcpy(res.px, src + sizeof(header), tex_size);
	}

error:;
	return res;
}

void
tex_clr(struct tex dst, i32 col)
{
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
	i32 wbyte      = tex.wword * 4;
	u32 *color_dst = (u32 *)px_out;
	u32 *mask_dst  = (u32 *)mask_out;
	const u32 *src = (const u32 *)tex.px;
	i32 y2         = MIN(h, tex.h);
	i32 x2         = MIN(row_bytes, wbyte) / 4;
	i32 stride     = row_bytes / 4;
	i32 dst_words  = ((w + 31) >> 5);

	for(i32 y = 0; y < y2; ++y) {
		for(i32 x = 0; x < x2; ++x) {
			u32 color_word            = *src++;
			u32 mask_word             = *src++;
			color_dst[y * stride + x] = color_word;
			if(mask_dst != NULL) {
				mask_dst[y * stride + x] = mask_word;
			}
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
	usize mem_size = sizeof(u32) * dst->wword * dst->h;
	mcpy(dst->px, src->px, mem_size);
}

struct tex
tex_from_rgb(struct alloc alloc, const struct pixel_u8 *in_data, i32 w, i32 h)
{
	struct tex t  = {0};
	b32 opaque    = true;
	u32 *dst      = NULL;
	i32 w_aligned = 0;
	ssize bit_idx = 0;
	u32 color_row = 0;
	u32 mask_row  = 0;

	// Any pixel with alpha <= 127 makes this a mask tex.
	for(i32 y = 0; y < h && opaque; ++y) {
		const struct pixel_u8 *row = in_data + y * w;
		for(i32 x = 0; x < w; ++x) {
			if(row[x].a <= 127) {
				opaque = false;
				break;
			}
		}
	}

	t = tex_create_internal(alloc, w, h, !opaque);
	dbg_check_mem(t.px, "tex");

	dst       = t.px;
	w_aligned = (w + 31) & ~31;

	for(ssize y = 0; y < h; ++y) {
		const struct pixel_u8 *row = in_data + y * w;

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
				*dst++ = bswap_u32(color_row);
				if(!opaque) {
					*dst++ = bswap_u32(mask_row);
				}

				color_row = 0;
				mask_row  = 0;
				bit_idx   = 0;
			}
		}

		// Flush leftover bits when width is not a multiple of 32.
		if(bit_idx > 0) {
			*dst++ = bswap_u32(color_row);
			if(!opaque) {
				*dst++ = bswap_u32(mask_row);
			}

			color_row = 0;
			mask_row  = 0;
			bit_idx   = 0;
		}
	}

error:;
	return t;
}
