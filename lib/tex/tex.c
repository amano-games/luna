#include "tex.h"
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

	void *mem = alloc.allocf(alloc.ctx, size);

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
	void *f = sys_file_open(path, SYS_FILE_MODE_R);
	if(f == NULL) {
		log_error("Assets", "failed to open texture %s", path.str);
		return (struct tex){0};
	}

	u32 w;
	u32 h;
	sys_file_r(f, &w, sizeof(uint));
	sys_file_r(f, &h, sizeof(uint));

	i32 width_alinged = (w + 31) & ~31;
	usize size        = ((width_alinged * h) * 2) / 8;

	struct tex t = tex_create(w, h, alloc);
	sys_file_r(f, t.px, size);
	sys_file_close(f);
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
	u32 b = bswap32(0x80000000U >> (x & 31));
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

	u32 b = bswap32(0x80000000U >> (x & 31));
	return (tex.px[y * tex.wword + ((x >> 5) << 1) + 1] & b);
}

static void
tex_px_unsafe(struct tex tex, i32 x, i32 y, i32 col)
{
	u32 b  = bswap32(0x80000000U >> (x & 31));
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
	u32 b  = bswap32(0x80000000U >> (x & 31));
	u32 *p = &tex.px[y * tex.wword + (x >> 5)];
	*p     = (col == 0 ? *p & ~b : *p | b);
}

static void
tex_mask_unsafe(struct tex tex, i32 x, i32 y, i32 col)
{
	if(tex.fmt == TEX_FMT_OPAQUE) return;
	u32 b  = bswap32(0x80000000U >> (x & 31));
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
tex_opaque_to_rgba(struct tex tex, u32 *out, size size, u32 white, u32 black)
{
	u32 *pixels       = out;
	i32 width_alinged = (tex.w + 31) & ~31;
	i32 wbytes        = width_alinged / 8;
	u8 *in            = (u8 *)tex.px;
	for(i32 y = 0; y < tex.h; y++) {
		for(i32 x = 0; x < tex.w; x++) {
			i32 src     = (x >> 3) + y * wbytes;
			i32 dst     = x + y * tex.w;
			i32 byt     = in[src];
			i32 bit     = !!(byt & 0x80 >> (x & 7));
			pixels[dst] = (bit ? white : black) | 0xFF000000;
		}
	}
}
