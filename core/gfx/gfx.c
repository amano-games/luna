#include "gfx.h"
#include "sys.h"
#include "sys-intrin.h"
#include "mathfunc.h"
#include "trace.h"

struct span_blit {
	u32 *dp;  // pixel
	u16 dmax; // count of dst words -1
	u16 dadd;
	u32 ml;   // boundary mask left
	u32 mr;   // boundary mask right
	i16 mode; // drawing mode
	i16 doff; // bitoffset of first dst bit
	u16 dst_wword;
	i16 y;
	struct gfx_pattern pat;
};

struct tex
tex_frame_buffer(void)
{
	struct tex t = {0};
	t.fmt        = TEX_FMT_OPAQUE;
	t.px         = (u32 *)sys_1bit_buffer();
	t.w          = SYS_DISPLAY_W;
	t.h          = SYS_DISPLAY_H;
	t.wword      = SYS_DISPLAY_WWORDS;
	return t;
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
	TRACE_START(__func__);
	static const u32 ditherpat[GFX_PATTERN_NUM * 4] = {
		0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U, 0x88888888U, 0x00000000U, 0x00000000U, 0x00000000U, 0x88888888U, 0x00000000U, 0x22222222U, 0x00000000U, 0xAAAAAAAAU, 0x00000000U, 0x22222222U, 0x00000000U, 0xAAAAAAAAU, 0x00000000U, 0xAAAAAAAAU, 0x00000000U, 0xAAAAAAAAU, 0x44444444U, 0xAAAAAAAAU, 0x00000000U, 0xAAAAAAAAU, 0x44444444U, 0xAAAAAAAAU, 0x11111111U, 0xAAAAAAAAU, 0x55555555U, 0xAAAAAAAAU, 0x11111111U, 0xAAAAAAAAU, 0x55555555U, 0xAAAAAAAAU, 0x55555555U, 0xEEEEEEEEU, 0x55555555U, 0xAAAAAAAAU, 0x55555555U, 0xEEEEEEEEU, 0x55555555U, 0xBBBBBBBBU, 0x55555555U, 0xFFFFFFFFU, 0x55555555U, 0xBBBBBBBBU, 0x55555555U, 0xFFFFFFFFU, 0x55555555U, 0xFFFFFFFFU, 0x55555555U, 0xFFFFFFFFU, 0xDDDDDDDDU, 0xFFFFFFFFU, 0x55555555U, 0xFFFFFFFFU, 0xDDDDDDDDU, 0xFFFFFFFFU, 0x77777777U, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0x77777777U, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU, 0xFFFFFFFFU};

	const u32 *p           = &ditherpat[clamp_i32(i, 0, GFX_PATTERN_MAX) << 2];
	struct gfx_pattern pat = {{p[0], p[1], p[2], p[3], p[0], p[1], p[2], p[3]}};
	TRACE_END();
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
	mset(&ctx.pat, 0xFF, sizeof(struct gfx_pattern));

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
span_blit_gen(struct gfx_ctx ctx, i32 y, i32 x1, i32 x2, enum prim_mode mode)
{
	i32 nbit              = (x2 + 1) - x1; // number of bits in a row to blit
	i32 lsh               = (ctx.dst.fmt == TEX_FMT_MASK);
	struct span_blit info = {0};
	info.y                = y;
	info.doff             = x1 & 31;
	info.dmax             = (info.doff + nbit - 1) >> 5;                        // number of touched dst words -1
	info.mode             = mode;                                               // sprite masking mode
	info.ml               = bswap32(0xFFFFFFFFU >> (31 & info.doff));           // mask to cut off boundary left
	info.mr               = bswap32(0xFFFFFFFFU << (31 & (-info.doff - nbit))); // mask to cut off boundary right
	info.dst_wword        = ctx.dst.wword;
	info.dp               = &ctx.dst.px[((x1 >> 5) << lsh) + y * ctx.dst.wword];
	info.dadd             = 1 + lsh;
	info.pat              = ctx.pat;
	return info;
}

static inline void
span_blit_incr_y(struct span_blit *info)
{
	info->y++;
	info->dp += info->dst_wword;
}

static void
apply_prim_mode(u32 *restrict dp, u32 *restrict dm, u32 sm, enum prim_mode mode, u32 pt)
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
prim_blit_span(struct span_blit info)
{
	u32 *restrict dp = (u32 *restrict)info.dp;
	u32 pt           = info.pat.p[info.y & 7];
	u32 m            = info.ml;
	for(i32 i = 0; i < info.dmax; i++) {
		apply_prim_mode(dp, info.dadd == 2 ? dp + 1 : NULL, m, info.mode, pt);
		m = 0xFFFFFFFFU;
		dp += info.dadd;
	}
	apply_prim_mode(dp, info.dadd == 2 ? dp + 1 : NULL, m & info.mr, info.mode, pt);
}

static void
apply_prim_mode_x(u32 *restrict dp, u32 sm, enum prim_mode mode, u32 pt)
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
prim_blit_span_x(struct span_blit info)
{
	u32 *restrict dp = (u32 *restrict)info.dp;
	u32 pt           = info.pat.p[info.y & 7];
	u32 m            = info.ml;
	for(i32 i = 0; i < info.dmax; i++) {
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
	for(i32 i = 0; i < info.dmax; i++) {
		apply_prim_mode(dp, dp + 1, m, info.mode, pt);
		m = 0xFFFFFFFFU;
		dp += 2;
	}
	apply_prim_mode(dp, dp + 1, m & info.mr, info.mode, pt);
}

void
gfx_rec(
	struct gfx_ctx ctx,
	i32 x,
	i32 y,
	i32 w,
	i32 h,
	enum prim_mode mode)
{
	i32 x2          = x + (w - 1);
	i32 y2          = y + (h - 1);
	v2_i32 verts[4] = {
		{x, y},
		{x2, y},
		{x2, y2},
		{x, y2},
	};

	gfx_poly(ctx, verts, 4, mode, 1);
}

void
gfx_rec_fill(struct gfx_ctx ctx, i32 x, i32 y, i32 w, i32 h, enum prim_mode mode)
{
	TRACE_START(__func__);
	i32 x1 = max_i32(x, ctx.clip_x1); // area bounds on canvas [x1/y1, x2/y2]
	i32 y1 = max_i32(y, ctx.clip_y1);
	i32 x2 = min_i32(x + w - 1, ctx.clip_x2);
	i32 y2 = min_i32(y + h - 1, ctx.clip_y2);
	if(x2 < x1) goto cleanup;

	dbg_assert(y2 <= ctx.clip_y2);
	struct tex dtex       = ctx.dst;
	struct span_blit info = span_blit_gen(ctx, y1, x1, x2, mode);
	if(dtex.fmt == TEX_FMT_OPAQUE) {
		for(i32 y = y1; y <= y2; y++) {
			prim_blit_span_x(info);
			span_blit_incr_y(&info);
		}
	} else {
		for(i32 y = y1; y <= y2; y++) {
			prim_blit_span_y(info);
			span_blit_incr_y(&info);
		}
	}

cleanup:
	TRACE_END();
}

void
gfx_rrec_fill(struct gfx_ctx ctx, i32 x, i32 y, i32 w, i32 h, i32 r, i32 mode)
{
	i32 rr = r < 0 ? h / 2 : min_i32(r, h / 2);
	i32 y1 = max_i32(ctx.clip_y1 - y, 0);
	i32 y2 = min_i32(ctx.clip_y2 - y, h - 1);

	for(i32 y0 = y1; y0 < rr; y0++) {
		i32 dx = rr - sqrt_u32(pow2_i32(rr) - pow2_i32(rr - y0) + 1);
		gfx_rec_fill(ctx, x + dx, y0 + y, w - (dx << 1), 1, mode);
	}

	gfx_rec_fill(ctx, x, y + rr, w, h - (rr << 1), mode);

	i32 t = h - rr - 1;
	for(i32 y0 = t + 1; y0 <= y2; y0++) {
		i32 dx = rr - sqrt_u32(pow2_i32(rr) - pow2_i32(y0 - t) + 1);
		gfx_rec_fill(ctx, x + dx, y0 + y, w - (dx << 1), 1, mode);
	}
}

void
gfx_fill_rows(struct tex dst, struct gfx_pattern pat, i32 y1, i32 y2)
{
	dbg_assert(0 <= y1 && y2 <= dst.h);
	u32 *px = &dst.px[y1 * dst.wword];
	for(i32 y = y1; y < y2; y++) {
		const u32 p = pat.p[y & 7];
		for(i32 x = 0; x < dst.wword; x++) {
			*px++ = p;
		}
	}
}

void
gfx_cir(struct gfx_ctx ctx, i32 px, i32 py, i32 d, enum prim_mode mode)
{
	if(d <= 0) return;
	if(d == 1) {
		tex_px(ctx.dst, px, py, mode);
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
			tex_px(ctx.dst, x3, y4, mode);
			tex_px(ctx.dst, x4, y4, mode);
		}
		if(ctx.clip_y1 <= y2 && y2 <= ctx.clip_y2 && x1 <= x2) {
			tex_px(ctx.dst, x1, y2, mode);
			tex_px(ctx.dst, x2, y2, mode);
		}
		if(ctx.clip_y1 <= y1 && y1 <= ctx.clip_y2 && x1 <= x2 && y != 0) {
			tex_px(ctx.dst, x1, y1, mode);
			tex_px(ctx.dst, x2, y1, mode);
		}
		if(ctx.clip_y1 <= y3 && y3 <= ctx.clip_y2 && x3 <= x4) {
			tex_px(ctx.dst, x3, y3, mode);
			tex_px(ctx.dst, x4, y3, mode);
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
gfx_cir_fill(
	struct gfx_ctx ctx,
	i32 px,
	i32 py,
	i32 d,
	enum prim_mode mode)
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
gfx_lin(struct gfx_ctx ctx, i32 ax, i32 ay, i32 bx, i32 by, enum prim_mode mode)
{
	gfx_lin_thick(ctx, ax, ay, bx, by, 1, mode);
}

void
gfx_lin_thick(struct gfx_ctx ctx, i32 ax, i32 ay, i32 bx, i32 by, i32 d, enum prim_mode mode)
{
#define GFX_LIN_NUM_SPANS 512
#define GFX_LIN_NUM_CIRX  64

	static u16 spans[GFX_LIN_NUM_SPANS][2];
	static u8 cirx[GFX_LIN_NUM_CIRX];

	i32 r = d >> 1;
	dbg_assert(r < GFX_LIN_NUM_CIRX);

	if(r <= 1) {
		cirx[0] = r;
		cirx[1] = r;
	} else {
		// Jesko's Method - schwarzers.com/algorithms
		mset(cirx, 0, r);
		i32 x = r;
		i32 y = 0;
		i32 t = r >> 4;

		do {
			cirx[y] = max_i32(cirx[y], x);
			cirx[x] = max_i32(cirx[x], y);
			y++;
			t += y;
			i32 k = t - x;
			if(0 <= k) {
				t = k;
				x--;
			}
		} while(y <= x);
	}

	i32 ymin = max_i32(min_i32(ay, by) - r, ctx.clip_y1);
	i32 ymax = min_i32(max_i32(ay, by) + r, ctx.clip_y2);
	i32 y_dt = ymax - ymin;
	dbg_assert(y_dt < GFX_LIN_NUM_SPANS);

	for(i32 n = 0; n <= y_dt; n++) {
		spans[n][0] = U16_MAX;
		spans[n][1] = 0;
	}

	i32 dx = +abs_i32(bx - ax);
	i32 dy = -abs_i32(by - ay);
	i32 sx = ax < bx ? +1 : -1;
	i32 sy = ay < by ? +1 : -1;
	i32 er = dx + dy;
	i32 xi = ax;
	i32 yi = ay;

	while(1) {
		for(i32 y = 0; y <= r; y++) {
			i32 x1 = max_i32(xi - (i32)cirx[y], ctx.clip_x1);
			i32 x2 = min_i32(xi + (i32)cirx[y], ctx.clip_x2);
			if(x2 < x1) continue;
			i32 y1 = yi - y - ymin;
			i32 y2 = yi + y - ymin;
			dbg_assert(0 <= x1 && x1 <= U16_MAX);
			dbg_assert(0 <= x2 && x2 <= U16_MAX);
			if(0 <= y1 && y1 <= y_dt) {

				spans[y1][0] = min_i32(spans[y1][0], x1);
				spans[y1][1] = max_i32(spans[y1][1], x2);
			}
			if(0 <= y2 && y2 <= y_dt) {
				spans[y2][0] = min_i32(spans[y2][0], x1);
				spans[y2][1] = max_i32(spans[y2][1], x2);
			}
		}

		if(xi == bx && yi == by) break;
		i32 e2 = er << 1;
		if(e2 >= dy) { er += dy, xi += sx; }
		if(e2 <= dx) { er += dx, yi += sy; }
	}

	for(i32 y = ymin; y <= ymax; y++) {
		i32 n  = y - ymin;
		i32 x1 = spans[n][0];
		i32 x2 = spans[n][1];
		if(x2 < x1) continue;
		struct span_blit info = span_blit_gen(ctx, y, x1, x2, mode);
		prim_blit_span(info);
	}
}

void
gfx_poly(
	struct gfx_ctx ctx,
	v2_i32 *verts,
	i32 count,
	i32 r,
	enum prim_mode mode)
{
	for(i32 i = 0; i < count; ++i) {
		v2_i32 a = verts[i];
		v2_i32 b = verts[(i + 1) % count];

		gfx_lin_thick(ctx, a.x, a.y, b.x, b.y, mode, r);
	}
}

// https://github.com/olikraus/u8g2/issues/2243
// https://motla.github.io/arc-algorithm/
void
gfx_arc(
	struct gfx_ctx ctx,
	i32 x0,
	i32 y0,
	u8 start_ang,
	u8 end_ang,
	i32 rad,
	enum prim_mode mode)
{
	u8 full     = (end_ang == start_ang);
	u8 inverted = end_ang > start_ang;
	u8 a_start  = inverted ? start_ang : end_ang;
	u8 a_end    = inverted ? end_ang : start_ang;

	u32 ratio;
	i32 x = 0;
	i32 y = rad;
	i32 d = rad - 1;

	// Trace arc radius with the Andres circle algorithm (process each pixel of a 1/8th circle of radius rad)
	while(y >= x) {
		// Get the percentage of 1/8th circle drawn with a fast approximation of arctan(x/y)
		ratio = x * 255 / y;                                                // x/y [0..255]
		ratio = ratio * (770195 - (ratio - 255) * (ratio + 941)) / 6137491; // arctan(x/y) [0..32] // Fill the pixels of the 8 sections of the circle, but only on the arc defined by the angles (start and end)
		if(full || ((ratio >= a_start && ratio < a_end) ^ inverted)) tex_px(ctx.dst, x0 + y, y0 - x, mode);
		if(full || (((ratio + a_end) > 63 && (ratio + a_start) <= 63) ^ inverted)) tex_px(ctx.dst, x0 + x, y0 - y, mode);
		if(full || (((ratio + 64) >= a_start && (ratio + 64) < a_end) ^ inverted)) tex_px(ctx.dst, x0 - x, y0 - y, mode);
		if(full || (((ratio + a_end) > 127 && (ratio + a_start) <= 127) ^ inverted)) tex_px(ctx.dst, x0 - y, y0 - x, mode);
		if(full || (((ratio + 128) >= a_start && (ratio + 128) < a_end) ^ inverted)) tex_px(ctx.dst, x0 - y, y0 + x, mode);
		if(full || (((ratio + a_end) > 191 && (ratio + a_start) <= 191) ^ inverted)) tex_px(ctx.dst, x0 - x, y0 + y, mode);
		if(full || (((ratio + 192) >= a_start && (ratio + 192) < a_end) ^ inverted)) tex_px(ctx.dst, x0 + x, y0 + y, mode);
		if(full || (((ratio + a_end) > 255 && (ratio + a_start) <= 255) ^ inverted)) tex_px(ctx.dst, x0 + y, y0 + x, mode);
		if(d >= 2 * x) {
			d = d - 2 * x - 1;
			x = x + 1;
		} else if(d < 2 * (rad - y)) {
			d = d + 2 * y - 1;
			y = y - 1;
		} else {
			d = d + 2 * (y - x - 1);
			y = y - 1;
			x = x + 1;
		}
	}
}

void
gfx_arc_thick(
	struct gfx_ctx ctx,
	i32 x0,
	i32 y0,
	u8 start,
	u8 end,
	i32 rad,
	i32 thick,
	enum prim_mode mode)
{
	// Draw arc for each radius
	for(i32 r = rad; r <= (rad + thick); r++) {
		gfx_arc(ctx, x0, y0, start, end, r, mode);
	}
}

static inline void
gfx_ellipse_section(
	struct gfx_ctx ctx,
	i32 x,
	i32 y,
	i32 x0,
	i32 y0,
	enum prim_mode mode)
{
	// Upper right
	tex_px(ctx.dst, x0 + x, y0 - y, mode);
	// Upper left
	tex_px(ctx.dst, x0 - x, y0 - y, mode);
	// Lower right
	tex_px(ctx.dst, x0 + x, y0 + y, mode);
	// Lower left
	tex_px(ctx.dst, x0 - x, y0 + y, mode);
}

// https : //github.com/olikraus/u8g2/blob/a549a13b13b5fd568111557c1dd7ca3d06fbe21a/csrc/u8g2_circle.c#L244
void
gfx_ellipsis(struct gfx_ctx ctx,
	i32 x0,
	i32 y0,
	i32 rx,
	i32 ry,
	enum prim_mode mode)
{
	i32 x, y;
	i32 xchg, ychg;
	i32 err;
	i32 rxrx2;
	i32 ryry2;
	i32 stopx, stopy;

	rxrx2 = rx;
	rxrx2 *= rx;
	rxrx2 *= 2;

	ryry2 = ry;
	ryry2 *= ry;
	ryry2 *= 2;

	x = rx;
	y = 0;

	xchg = 1;
	xchg -= rx;
	xchg -= rx;
	xchg *= ry;
	xchg *= ry;

	ychg = rx;
	ychg *= rx;

	err = 0;

	stopx = ryry2;
	stopx *= rx;
	stopy = 0;

	while(stopx >= stopy) {
		gfx_ellipse_section(ctx, x, y, x0, y0, mode);
		y++;
		stopy += rxrx2;
		err += ychg;
		ychg += rxrx2;
		if(2 * err + xchg > 0) {
			x--;
			stopx -= ryry2;
			err += xchg;
			xchg += ryry2;
		}
	}

	x = 0;
	y = ry;

	xchg = ry;
	xchg *= ry;

	ychg = 1;
	ychg -= ry;
	ychg -= ry;
	ychg *= rx;
	ychg *= rx;

	err = 0;

	stopx = 0;

	stopy = rxrx2;
	stopy *= ry;

	while(stopx <= stopy) {
		gfx_ellipse_section(ctx, x, y, x0, y0, mode);
		x++;
		stopx += ryry2;
		err += xchg;
		xchg += ryry2;
		if(2 * err + ychg > 0) {
			y--;
			stopy -= rxrx2;
			err += ychg;
			ychg += rxrx2;
		}
	}
}
