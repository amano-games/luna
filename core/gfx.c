#include <stdio.h>
#include <string.h>

#include "gfx.h"
#include "sys-log.h"
#include "sys.h"
#include "sys-intrin.h"
#include "sys-io.h"
#include "mathfunc.h"
#include "trace.h"

struct span_blit {
	int dmax; // count of dst words -1
	u32 ml;   // boundary mask left
	u32 mr;   // boundary mask right
	int mode; // drawing mode
	int doff; // bitoffset of first dst bit
	u32 *dp;  // pixel
	int y;
	struct gfx_pattern pat;
	int dadd;
};

struct tex
tex_frame_buffer(void)
{
	struct sys_display d = sys_display();
	struct tex t         = {0};
	t.fmt                = TEX_FMT_OPAQUE;
	t.px                 = d.px;
	t.w                  = d.w;
	t.h                  = d.h;
	t.wword              = d.wword;

	return t;
}

// mask is almost always = 1
struct tex
tex_create_in(int w, int h, bool32 mask, struct alloc ma)
{
	struct tex t = {0};

	// NOTICE: Seems that the tex should be padded on creation
	// If not the correct size won't be calculated
	// Align the next multiple of 32 greater than or equal to the
	int width_alinged = (w + 31) & ~31;

	// Calculates the number of words needed for the width of the texture
	// It multiplies by 2 if it uses a mask (transparency)
	// by shifting it by 1 << (0 < mask)
	int width_word = (width_alinged / 32) << (0 < mask);

	// So each `word` is a row of pixels aligned
	// To get the size we multiply by the height
	// getting the full size of the image aligned.
	usize size = sizeof(u32) * width_word * h * 2;

	void *mem = ma.allocf(ma.ctx, size);

	if(!mem) return t;

	t.px    = (u32 *)mem;
	t.fmt   = (0 < mask);
	t.w     = w;
	t.h     = h;
	t.wword = width_word;
	return t;
}

struct tex
tex_create(int w, int h, struct alloc ma)
{
	return tex_create_in(w, h, 1, ma);
}

struct tex
tex_load(str8 path, struct alloc alloc)
{
	void *f = sys_file_open(path, SYS_FILE_R);
	if(f == NULL) {
		log_error("Assets", "failed to open texture %s", path.str);
		return (struct tex){0};
	}

	uint w;
	uint h;
	sys_file_read(f, &w, sizeof(uint));
	sys_file_read(f, &h, sizeof(uint));

	int width_alinged = (w + 31) & ~31;
	usize size        = ((width_alinged * h) * 2) / 8;

	struct tex t = tex_create(w, h, alloc);
	sys_file_read(f, t.px, size);
	sys_file_close(f);
	return t;
}

void
tex_clr(struct tex dst, int col)
{
	TRACE_START(__func__);
	int n  = dst.wword * dst.h;
	u32 *p = dst.px;
	switch(col) {
	case TEX_CLR_BLACK:
		// TODO: Handle transparent images
		for(int i = 0; i < n; i++) {
			*p++ = 0xFFFFFFFFU;
		}
		break;
	case TEX_CLR_WHITE:
		switch(dst.fmt) {
		case TEX_FMT_OPAQUE:
			for(int i = 0; i < n; i++) {
				*p++ = 0U;
			}
			break;
		case TEX_FMT_MASK:
			for(int i = 0; i < n; i += 2) {
				*p++ = 0;           // data
				*p++ = 0xFFFFFFFFU; // mask
			}
			break;
		}
		break;
	case TEX_CLR_TRANSPARENT:
		if(dst.fmt == TEX_FMT_OPAQUE) break;
		for(int i = 0; i < n; i++) {
			*p++ = 0;
		}
		break;
	}

	TRACE_END();
}

struct gfx_pattern
gfx_pattern_2x2(i32 p0, i32 p1)
{
	struct gfx_pattern pat = {0};
	i32 p[2]               = {p0, p1};
	for(i32 i = 0; i < 2; i++) {
		u32 pp       = (u32)p[i];
		u32 pa       = (pp << 6) | (pp << 4) | (pp << 2) | (pp);
		u32 pb       = (pa << 24) | (pa << 16) | (pa << 8) | (pa);
		pat.p[i + 0] = pb;
		pat.p[i + 2] = pb;
		pat.p[i + 4] = pb;
		pat.p[i + 6] = pb;
	}
	return pat;
}

struct gfx_pattern
gfx_pattern_4x4(i32 p0, i32 p1, i32 p2, i32 p3)
{
	struct gfx_pattern pat = {0};
	u32 p[4]               = {p0, p1, p2, p3};
	for(i32 i = 0; i < 4; i++) {
		u32 pa       = ((u32)p[i] << 4) | ((u32)p[i]);
		u32 pb       = (pa << 24) | (pa << 16) | (pa << 8) | (pa);
		pat.p[i + 0] = pb;
		pat.p[i + 4] = pb;
	}
	return pat;
}

struct gfx_pattern
gfx_pattern_8x8(i32 p0, i32 p1, i32 p2, i32 p3, i32 p4, i32 p5, i32 p6, i32 p7)
{
	struct gfx_pattern pat = {0};
	u32 p[8]               = {p0, p1, p2, p3, p4, p5, p6, p7};
	for(i32 i = 0; i < 8; i++) {
		u32 pp   = (u32)p[i];
		pat.p[i] = (pp << 24) | (pp << 16) | (pp << 8) | (pp);
	}
	return pat;
}

struct gfx_pattern
gfx_pattern_bayer_4x4(i32 i)
{
	static const u32 ditherpat[GFX_PATTERN_NUM * 4] = {
		0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x88888888U, 0x00000000U, 0x00000000U, 0x00000000U, 0x88888888U, 0x00000000U, 0x22222222U, 0x00000000U, 0xAAAAAAAAU, 0x00000000U, 0x22222222U, 0x00000000U, 0xAAAAAAAAU, 0x00000000U, 0xAAAAAAAAU, 0x00000000U, 0xAAAAAAAAU, 0x44444444U, 0xAAAAAAAAU, 0x00000000U, 0xAAAAAAAAU, 0x44444444U, 0xAAAAAAAAU, 0x11111111U, 0xAAAAAAAAU, 0x55555555U, 0xAAAAAAAAU, 0x11111111U, 0xAAAAAAAAU, 0x55555555U, 0xAAAAAAAAU, 0x55555555U, 0xEEEEEEEEU, 0x55555555U, 0xAAAAAAAAU, 0x55555555U, 0xEEEEEEEEU, 0x55555555U, 0xBBBBBBBBU, 0x55555555U, 0xFFFFFFFFU, 0x55555555U, 0xBBBBBBBBU, 0x55555555U, 0xFFFFFFFFU, 0x55555555U, 0xFFFFFFFFU, 0x55555555U, 0xFFFFFFFFU, 0xDDDDDDDDU, 0xFFFFFFFFU, 0x55555555U, 0xFFFFFFFFU, 0xDDDDDDDDU, 0xFFFFFFFFU, 0x77777777U, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0x77777777U, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU};

	const u32 *p           = &ditherpat[clamp_i32(i, 0, GFX_PATTERN_MAX) << 2];
	struct gfx_pattern pat = {{p[0], p[1], p[2], p[3], p[0], p[1], p[2], p[3]}};
	return pat;
}

struct gfx_pattern
gfx_pattern_interpolate(i32 num, i32 den)
{
	return gfx_pattern_bayer_4x4((num * GFX_PATTERN_NUM) / den);
}

struct gfx_pattern
gfx_pattern_interpolatec(i32 num, i32 den, i32 (*ease)(i32 a, i32 b, i32 num, i32 den))
{
	i32 i = ease(0, 16, num, den);
	return gfx_pattern_bayer_4x4(i);
}

struct gfx_ctx
gfx_ctx_default(struct tex dst)
{
	struct gfx_ctx ctx = {0};
	ctx.dst            = dst;
	ctx.clip_x2        = dst.w - 1;
	ctx.clip_y2        = dst.h - 1;
	memset(&ctx.pat, 0xFF, sizeof(struct gfx_pattern));

	return ctx;
}

struct gfx_ctx
gfx_ctx_display(void)
{
	TRACE_START(__func__);
	struct gfx_ctx ctx = gfx_ctx_default(tex_frame_buffer());
	TRACE_END();
	return ctx;
}

struct gfx_ctx
gfx_ctx_unclip(struct gfx_ctx ctx)
{
	struct gfx_ctx c = ctx;
	c.clip_x1        = 0;
	c.clip_y1        = 0;
	ctx.clip_x2      = ctx.dst.w - 1;
	ctx.clip_y2      = ctx.dst.h - 1;
	return c;
}

struct gfx_ctx
gfx_ctx_clip(struct gfx_ctx ctx, i32 x1, i32 y1, i32 x2, i32 y2)
{
	struct gfx_ctx c = ctx;
	c.clip_x1        = max_i32(x1, 0);
	c.clip_y1        = max_i32(y1, 0);
	c.clip_x2        = min_i32(x2, ctx.dst.w - 1);
	c.clip_y2        = min_i32(y2, ctx.dst.h - 1);
	return c;
}

struct gfx_ctx
gfx_ctx_clip_top(struct gfx_ctx ctx, i32 y1)
{
	struct gfx_ctx c = ctx;
	c.clip_y1        = max_i32(y1, 0);
	return c;
}

struct gfx_ctx
gfx_ctx_clip_bot(struct gfx_ctx ctx, i32 y2)
{
	struct gfx_ctx c = ctx;
	c.clip_y2        = min_i32(y2, ctx.dst.h - 1);
	return c;
}

struct gfx_ctx
gfx_ctx_clip_left(struct gfx_ctx ctx, i32 x1)
{
	struct gfx_ctx c = ctx;
	c.clip_x1        = max_i32(x1, 0);
	return c;
}

struct gfx_ctx
gfx_ctx_clip_right(struct gfx_ctx ctx, i32 x2)
{
	struct gfx_ctx c = ctx;
	c.clip_x2        = min_i32(x2, ctx.dst.w - 1);
	return c;
}

struct gfx_ctx
gfx_ctx_clipr(struct gfx_ctx ctx, rec_i32 r)
{
	return gfx_ctx_clip(ctx, r.x, r.y, r.x + r.w - 1, r.x + r.h - 1);
}

struct gfx_ctx
gfx_ctx_clipwh(struct gfx_ctx ctx, i32 x, i32 y, i32 w, i32 h)
{
	return gfx_ctx_clip(ctx, x, y, x + w - 1, x + h - 1);
}

struct span_blit
span_blit_gen(struct gfx_ctx ctx, int y, int x1, int x2, int mode)
{
	int nbit = (x2 + 1) - x1; // number of bits in a row to blit

	struct span_blit info = {0};
	info.y                = y;
	info.doff             = x1 & 31;
	info.dmax             = (info.doff + nbit - 1) >> 5;
	info.mode             = mode;
	info.ml               = bswap32(0xFFFFFFFFU >> (31 & info.doff));           // mask to cut off boundary left
	info.mr               = bswap32(0xFFFFFFFFU << (31 & (-info.doff - nbit))); // mask to cut off boundary right
	info.dp               = &ctx.dst.px[(x1 >> 5) + y * ctx.dst.wword];
	info.dadd             = 1 + (ctx.dst.fmt == TEX_FMT_MASK);
	info.pat              = ctx.pat;

	return info;
}

static void
apply_prim_mode(u32 *restrict dp, u32 *restrict dm, u32 sm, int mode, u32 pt)
{
	switch(mode) {
	case PRIM_MODE_INV: sm &= pt, *dp = (*dp & ~sm) | (~*dp & sm); break;
	case PRIM_MODE_WHITE: sm &= pt, *dp |= sm; break;
	case PRIM_MODE_BLACK: sm &= pt, *dp &= ~sm; break;
	case PRIM_MODE_WHITE_BLACK: pt = ~pt; // fallthrough
	case PRIM_MODE_BLACK_WHITE: *dp = (*dp & ~(sm & pt)) | (sm & ~pt); break;
	}

	if(dm) *dm |= sm;
}

static void
apply_prim_mode_x(u32 *restrict dp, u32 sm, int mode, u32 pt)
{
	switch(mode) {
	case PRIM_MODE_INV: sm &= pt, *dp = (*dp & ~sm) | (~*dp & sm); break;
	case PRIM_MODE_WHITE: sm &= pt, *dp |= sm; break;
	case PRIM_MODE_BLACK: sm &= pt, *dp &= ~sm; break;
	case PRIM_MODE_WHITE_BLACK: pt = ~pt; // fallthrough
	case PRIM_MODE_BLACK_WHITE: *dp = (*dp & ~(sm & pt)) | (sm & ~pt); break;
	}
}

static void
prim_blit_span(struct span_blit info)
{
	u32 *restrict dp = (u32 *restrict)info.dp;
	u32 pt           = info.pat.p[info.y & 7];
	u32 m            = info.ml;
	for(int i = 0; i < info.dmax; i++) {
		apply_prim_mode(dp, info.dadd == 2 ? dp + 1 : NULL, m, info.mode, pt);
		m = 0xFFFFFFFFU;
		dp += info.dadd;
	}
	apply_prim_mode(dp, info.dadd == 2 ? dp + 1 : NULL, m & info.mr, info.mode, pt);
}

static void
prim_blit_span_x(struct span_blit info)
{
	u32 *restrict dp = (u32 *restrict)info.dp;
	u32 pt           = info.pat.p[info.y & 7];
	u32 m            = info.ml;

	for(int i = 0; i < info.dmax; i++) {
		apply_prim_mode_x(dp, m, info.mode, pt);
		m = 0xFFFFFFFFU;
		dp++;
	}
	apply_prim_mode_x(dp, m & info.mr, info.mode, pt);
}

static void
prim_blit_span_y(struct span_blit info)
{
	u32 *restrict dp = (u32 *restrict)info.dp;
	u32 pt           = info.pat.p[info.y & 7];
	u32 m            = info.ml;

	for(int i = 0; i < info.dmax; i++) {
		apply_prim_mode(dp, dp + 1, m, info.mode, pt);
		m = 0xFFFFFFFFU;
		dp += 2;
	}
	apply_prim_mode(dp, dp + 1, m & info.mr, info.mode, pt);
}

void
gfx_px(struct gfx_ctx ctx, int x, int y, int mode)
{
	gfx_rec_fill(ctx, x, y, 1, 1, mode);
}

void
gfx_rec(struct gfx_ctx ctx, rec_i32 rec, int mode, int r)
{
	v2_i32 verts[4] = {{rec.x, rec.y},
		{rec.x + rec.w, rec.y},
		{rec.x + rec.w, rec.y + rec.h},
		{rec.x, rec.y + rec.h}};

	gfx_poly(ctx, verts, 4, mode, r);
}

void
gfx_rec_fill(struct gfx_ctx ctx, i32 x, i32 y, i32 w, i32 h, int mode)
{
	int x1 = max_i32(x, ctx.clip_x1);
	int y1 = max_i32(y, ctx.clip_y1);

	int x2 = min_i32(x + w - 1, ctx.clip_x2);
	int y2 = min_i32(y + h - 1, ctx.clip_y2);

	if(x2 < x1) return;

	struct tex dtex       = ctx.dst;
	struct span_blit info = span_blit_gen(ctx, y1, x1, x2, mode);

	if(dtex.fmt == TEX_FMT_OPAQUE) {
		for(info.y = y1; info.y <= y2; info.y++) {
			prim_blit_span_x(info);
			info.dp += dtex.wword;
		}
	} else {
		for(info.y = y1; info.y <= y2; info.y++) {
			prim_blit_span_y(info);
			info.dp += dtex.wword;
		}
	}
}

void
gfx_cir(struct gfx_ctx ctx, int px, int py, int d, int mode)
{
	if(d <= 0) return;
	if(d == 1) {
		gfx_px(ctx, px, py, mode);
		return;
	}
	if(d == 2) {
		gfx_rec_fill(ctx, px - 1, py - 1, 2, 2, mode);
		return;
	}

	// Jesko's Method, shameless copy
	// https://schwarzers.com/algorithms/
	i32 r = d >> 1;
	i32 x = r;
	i32 y = 0;
	i32 t = r >> 4;

	do {
		i32 x1 = max_i32(px - x, ctx.clip_x1);
		i32 x2 = min_i32(px + x, ctx.clip_x2);
		i32 x3 = max_i32(px - y, ctx.clip_x1);
		i32 x4 = min_i32(px + y, ctx.clip_x2);
		i32 y4 = py - x; // ordered in y
		i32 y2 = py - y;
		i32 y1 = py + y;
		i32 y3 = py + x;

		if(ctx.clip_y1 <= y4 && y4 <= ctx.clip_y2 && x3 <= x4) {
			gfx_px(ctx, x3, y4, mode);
			gfx_px(ctx, x4, y4, mode);
		}
		if(ctx.clip_y1 <= y2 && y2 <= ctx.clip_y2 && x1 <= x2) {
			gfx_px(ctx, x1, y2, mode);
			gfx_px(ctx, x2, y2, mode);
		}
		if(ctx.clip_y1 <= y1 && y1 <= ctx.clip_y2 && x1 <= x2 && y != 0) {
			gfx_px(ctx, x1, y1, mode);
			gfx_px(ctx, x2, y1, mode);
		}
		if(ctx.clip_y1 <= y3 && y3 <= ctx.clip_y2 && x3 <= x4) {
			gfx_px(ctx, x3, y3, mode);
			gfx_px(ctx, x4, y3, mode);
		}

		y++;
		t += y;
		i32 k = t - x;
		if(0 <= k) {
			t = k;
			x--;
		}
	} while(y <= x);
}

void
gfx_cir_fill(struct gfx_ctx ctx, int px, int py, int d, int mode)
{
	if(d <= 0) return;

	i32 r = d >> 1;

	switch(d) {
	case 1:
	case 2:
		gfx_rec_fill(ctx, px - r, py - r, d, d, mode);
		return;
	default: break;
	}

	// Jesko's Method, shameless copy
	// https://schwarzers.com/algorithms/

	i32 x = r;
	i32 y = 0;
	i32 t = r >> 4;

	do {
		i32 x1 = max_i32(px - x, ctx.clip_x1);
		i32 x2 = min_i32(px + x, ctx.clip_x2);
		i32 x3 = max_i32(px - y, ctx.clip_x1);
		i32 x4 = min_i32(px + y, ctx.clip_x2);
		i32 y4 = py - x; // ordered in y
		i32 y2 = py - y;
		i32 y1 = py + y;
		i32 y3 = py + x;

		if(ctx.clip_y1 <= y4 && y4 <= ctx.clip_y2 && x3 <= x4) {
			prim_blit_span(span_blit_gen(ctx, y4, x3, x4, mode));
		}
		if(ctx.clip_y1 <= y2 && y2 <= ctx.clip_y2 && x1 <= x2) {
			prim_blit_span(span_blit_gen(ctx, y2, x1, x2, mode));
		}
		if(ctx.clip_y1 <= y1 && y1 <= ctx.clip_y2 && x1 <= x2 && y != 0) {
			prim_blit_span(span_blit_gen(ctx, y1, x1, x2, mode));
		}
		if(ctx.clip_y1 <= y3 && y3 <= ctx.clip_y2 && x3 <= x4) {
			prim_blit_span(span_blit_gen(ctx, y3, x3, x4, mode));
		}

		y++;
		t += y;
		i32 k = t - x;
		if(0 <= k) {
			t = k;
			x--;
		}
	} while(y <= x);
}

void
gfx_lin(struct gfx_ctx ctx, i32 ax, i32 ay, i32 bx, i32 by, int mode)
{
	gfx_lin_thick(ctx, ax, ay, bx, by, mode, 1);
}

void
gfx_lin_thick(struct gfx_ctx ctx, i32 ax, i32 ay, i32 bx, i32 by, int mode, int r)
{
	int dx = +abs_i32(bx - ax);
	int dy = -abs_i32(by - ay);
	int sx = ax < bx ? +1 : -1;
	int sy = ay < by ? +1 : -1;
	int er = dx + dy;

	v2_i32 pi = {ax, ay};

	while(1) {
		gfx_cir_fill(ctx, pi.x, pi.y, r, mode);

		if(pi.x == bx && pi.y == by) break;
		int e2 = er << 1;
		if(e2 >= dy) {
			er += dy, pi.x += sx;
		}
		if(e2 <= dx) {
			er += dx, pi.y += sy;
		}
	}
}

void
gfx_poly(struct gfx_ctx ctx, v2_i32 *verts, int count, int mode, int r)
{
	for(int i = 0; i < count; ++i) {
		v2_i32 a = verts[i];
		v2_i32 b = verts[(i + 1) % count];

		gfx_lin_thick(ctx, a.x, a.y, b.x, b.y, mode, r);
	}
}

// Helper function to calculate the angle of a point (x, y) relative to the center (cx, cy)
static inline f32
calculate_angle(int cx, int cy, int x, int y)
{
	f32 angle = atan2_f32(y - cy, x - cx);
	if(angle < 0) angle += PI2_FLOAT; // Normalize angle to [0, 2*PI]
	return angle;
}

// Function to check if an angle is within the given range
static inline int
is_angle_in_range(f32 angle, f32 start_angle, f32 end_angle)
{
	if(start_angle < end_angle) {
		return (angle >= start_angle && angle <= end_angle);
	} else {
		return (angle >= start_angle || angle <= end_angle);
	}
}

static inline void
gfx_arc_plot(
	struct gfx_ctx ctx,
	i32 px,
	i32 py,
	i32 x,
	i32 y,
	f32 sa,
	f32 ea,
	i32 thick,
	int mode)
{
	f32 angle = calculate_angle(px, py, x, y);
	if(is_angle_in_range(angle, sa, ea)) {
		if(thick == 1) {
			gfx_px(ctx, x, y, mode);
		} else {
			gfx_cir_fill(ctx, x, y, thick, mode);
		}
	}
}

void
gfx_arc(
	struct gfx_ctx ctx,
	i32 px,
	i32 py,
	f32 sa,
	f32 ea,
	i32 d,
	int mode)
{
	if(d <= 0) return;
	if(d == 1) {
		gfx_px(ctx, px, py, mode);
		return;
	}
	if(d == 2) {
		gfx_rec_fill(ctx, px - 1, py - 1, 2, 2, mode);
		return;
	}

	// Jesko's Method, shameless copy
	// https://schwarzers.com/algorithms/
	i32 r = d >> 1;
	i32 x = r;
	i32 y = 0;
	i32 t = r >> 4;

	do {
		i32 x1 = max_i32(px - x, ctx.clip_x1);
		i32 x2 = min_i32(px + x, ctx.clip_x2);
		i32 x3 = max_i32(px - y, ctx.clip_x1);
		i32 x4 = min_i32(px + y, ctx.clip_x2);
		i32 y4 = py - x; // ordered in y
		i32 y2 = py - y;
		i32 y1 = py + y;
		i32 y3 = py + x;

		if(ctx.clip_y1 <= y4 && y4 <= ctx.clip_y2 && x3 <= x4) {
			gfx_arc_plot(ctx, px, py, x3, y4, sa, ea, 1, mode);
			gfx_arc_plot(ctx, px, py, x4, y4, sa, ea, 1, mode);
		}
		if(ctx.clip_y1 <= y2 && y2 <= ctx.clip_y2 && x1 <= x2) {
			gfx_arc_plot(ctx, px, py, x1, y2, sa, ea, 1, mode);
			gfx_arc_plot(ctx, px, py, x2, y2, sa, ea, 1, mode);
		}
		if(ctx.clip_y1 <= y1 && y1 <= ctx.clip_y2 && x1 <= x2 && y != 0) {
			gfx_arc_plot(ctx, px, py, x1, y1, sa, ea, 1, mode);
			gfx_arc_plot(ctx, px, py, x2, y1, sa, ea, 1, mode);
		}
		if(ctx.clip_y1 <= y3 && y3 <= ctx.clip_y2 && x3 <= x4) {
			gfx_arc_plot(ctx, px, py, x3, y3, sa, ea, 1, mode);
			gfx_arc_plot(ctx, px, py, x4, y3, sa, ea, 1, mode);
		}

		y++;
		t += y;
		i32 k = t - x;
		if(0 <= k) {
			t = k;
			x--;
		}
	} while(y <= x);
}

void
gfx_arc_thick(
	struct gfx_ctx ctx,
	i32 px,
	i32 py,
	f32 sa,
	f32 ea,
	i32 d,
	i32 thick,
	int mode)
{
	if(d <= 0) return;
	if(d == 1) {
		gfx_cir_fill(ctx, px, py, thick, mode);
		return;
	}
	if(d == 2) {
		gfx_rec_fill(ctx, px - 1, py - 1, 2, 2, mode);
		return;
	}

	// Jesko's Method, shameless copy
	// https://schwarzers.com/algorithms/
	i32 r = d >> 1;
	i32 x = r;
	i32 y = 0;
	i32 t = r >> 4;

	do {
		i32 x1 = max_i32(px - x, ctx.clip_x1);
		i32 x2 = min_i32(px + x, ctx.clip_x2);
		i32 x3 = max_i32(px - y, ctx.clip_x1);
		i32 x4 = min_i32(px + y, ctx.clip_x2);
		i32 y4 = py - x; // ordered in y
		i32 y2 = py - y;
		i32 y1 = py + y;
		i32 y3 = py + x;

		if(ctx.clip_y1 <= y4 && y4 <= ctx.clip_y2 && x3 <= x4) {
			gfx_arc_plot(ctx, px, py, x3, y4, sa, ea, thick, mode);
			gfx_arc_plot(ctx, px, py, x4, y4, sa, ea, thick, mode);
		}
		if(ctx.clip_y1 <= y2 && y2 <= ctx.clip_y2 && x1 <= x2) {
			gfx_arc_plot(ctx, px, py, x1, y2, sa, ea, thick, mode);
			gfx_arc_plot(ctx, px, py, x2, y2, sa, ea, thick, mode);
		}
		if(ctx.clip_y1 <= y1 && y1 <= ctx.clip_y2 && x1 <= x2 && y != 0) {
			gfx_arc_plot(ctx, px, py, x1, y1, sa, ea, thick, mode);
			gfx_arc_plot(ctx, px, py, x2, y1, sa, ea, thick, mode);
		}
		if(ctx.clip_y1 <= y3 && y3 <= ctx.clip_y2 && x3 <= x4) {
			gfx_arc_plot(ctx, px, py, x3, y3, sa, ea, thick, mode);
			gfx_arc_plot(ctx, px, py, x4, y3, sa, ea, thick, mode);
		}

		y++;
		t += y;
		i32 k = t - x;
		if(0 <= k) {
			t = k;
			x--;
		}
	} while(y <= x);
}
