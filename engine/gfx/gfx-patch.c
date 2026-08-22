#include "gfx-patch.h"
#include "base/dbg.h"
#include "gfx-spr.h"

enum {
	GFX_SPR_TILED_STAMP_W     = 32,
	GFX_SPR_TILED_STAMP_H_MAX = 64,
};

static void
gfx_spr_tiled_fill(struct gfx_ctx ctx, struct tex_rec src, rec_i32 dst, i32 flip, i32 mode)
{
	i32 tile_w = src.r.w;
	i32 tile_h = src.r.h;
	i32 x_end  = dst.x + dst.w;
	i32 y_end  = dst.y + dst.h;

	for(i32 y = dst.y; y < y_end; y += tile_h) {
		for(i32 x = dst.x; x < x_end; x += tile_w) {
			i32 rem_w              = x_end - x;
			i32 rem_h              = y_end - y;
			struct tex_rec clipped = src;

			clipped.r.w = rem_w < tile_w ? rem_w : tile_w;
			clipped.r.h = rem_h < tile_h ? rem_h : tile_h;
			gfx_spr(ctx, clipped, x, y, flip, mode);
		}
	}
}

void
gfx_spr_tiled(
	struct gfx_ctx ctx,
	struct tex_rec src,
	rec_i32 dst,
	i32 flip,
	i32 mode)
{
	if(!src.t.px || src.r.w <= 0 || src.r.h <= 0 || dst.w <= 0 || dst.h <= 0) {
		return;
	}

	i32 tile_w = src.r.w;
	i32 tile_h = src.r.h;
	b32 stamp  = tile_w > 0 &&
		tile_w < GFX_SPR_TILED_STAMP_W &&
		(GFX_SPR_TILED_STAMP_W % tile_w) == 0 &&
		tile_h > 0 &&
		tile_h <= GFX_SPR_TILED_STAMP_H_MAX &&
		dst.w >= GFX_SPR_TILED_STAMP_W &&
		!(flip & SPR_FLIP_X);

	if(!stamp) {
		gfx_spr_tiled_fill(ctx, src, dst, flip, mode);
	} else {
		// Repeat a narrow tile into a 32 px strip, then tile that.
		// Same pixels as stepping by tile_w;
		// dest words get one blit instead of ~8 overlapping 4 px RMWs.
		// Y-flip is applied below.
		u32 stamp_px[2 * GFX_SPR_TILED_STAMP_H_MAX];
		mclr(stamp_px, sizeof(stamp_px));

		struct tex stamp_tex = {
			.px    = stamp_px,
			.w     = GFX_SPR_TILED_STAMP_W,
			.h     = tile_h,
			.fmt   = src.t.fmt,
			.wword = src.t.fmt == TEX_FMT_MASK ? 2 : 1,
		};
		struct gfx_ctx stamp_ctx = gfx_ctx_default(stamp_tex);
		for(i32 x = 0; x < GFX_SPR_TILED_STAMP_W; x += tile_w) {
			gfx_spr(stamp_ctx, src, x, 0, 0, mode);
		}

		struct tex_rec stamp_src = {
			.t = stamp_tex,
			.r = {0, 0, GFX_SPR_TILED_STAMP_W, tile_h},
		};
		gfx_spr_tiled_fill(ctx, stamp_src, dst, flip, mode);
	}
}

void
gfx_patch(
	struct gfx_ctx ctx,
	struct tex_patch patch,
	i32 dx,
	i32 dy,
	i32 dw,
	i32 dh,
	enum spr_flip flip,
	enum spr_mode mode)
{
	i32 ml = patch.ml;
	i32 mr = patch.mr;
	i32 mt = patch.mt;
	i32 mb = patch.mb;
	i32 sx = patch.r.x;
	i32 sy = patch.r.y;
	i32 sw = patch.r.w;
	i32 sh = patch.r.h;

	dbg_assert(ml >= 0 && ml <= sw);
	dbg_assert(mr >= 0 && mr <= sw);
	dbg_assert(mt >= 0 && mt <= sh);
	dbg_assert(mb >= 0 && mb <= sh);

	if(ml > 0 && mt > 0) {
		struct tex_rec top_left = {.t = patch.t, .r = {sx, sy, ml, mt}};
		gfx_spr(ctx, top_left, dx, dy, flip, mode);
	}
	if(mr > 0 && mt > 0) {
		struct tex_rec top_right = {.t = patch.t, .r = {sx + sw - mr, sy, mr, mt}};
		gfx_spr(ctx, top_right, dx + dw - mr, dy, flip, mode);
	}
	if(ml > 0 && mb > 0) {
		struct tex_rec bottom_left = {.t = patch.t, .r = {sx, sy + sh - mb, ml, mb}};
		gfx_spr(ctx, bottom_left, dx, dy + dh - mb, flip, mode);
	}
	if(mr > 0 && mb > 0) {
		struct tex_rec bottom_right = {.t = patch.t, .r = {sx + sw - mr, sy + sh - mb, mr, mb}};
		gfx_spr(ctx, bottom_right, dx + dw - mr, dy + dh - mb, flip, mode);
	}

	i32 smw = sw - ml - mr;
	i32 smh = sh - mt - mb;
	i32 dmw = dw - ml - mr;
	i32 dmh = dh - mt - mb;

	if(mt > 0 && dmw > 0 && smw > 0) {
		rec_i32 dst_top    = {dx + ml, dy, dmw, mt};
		struct tex_rec top = {.t = patch.t, .r = {sx + ml, sy, smw, mt}};
		gfx_spr_tiled(ctx, top, dst_top, flip, mode);
	}
	if(mb > 0 && dmw > 0 && smw > 0) {
		rec_i32 dst_bottom    = {dx + ml, dy + dh - mb, dmw, mb};
		struct tex_rec bottom = {.t = patch.t, .r = {sx + ml, sy + sh - mb, smw, mb}};
		gfx_spr_tiled(ctx, bottom, dst_bottom, flip, mode);
	}
	if(ml > 0 && dmh > 0 && smh > 0) {
		rec_i32 dst_left    = {dx, dy + mt, ml, dmh};
		struct tex_rec left = {.t = patch.t, .r = {sx, sy + mt, ml, smh}};
		gfx_spr_tiled(ctx, left, dst_left, flip, mode);
	}
	if(mr > 0 && dmh > 0 && smh > 0) {
		rec_i32 dst_right    = {dx + dw - mr, dy + mt, mr, dmh};
		struct tex_rec right = {.t = patch.t, .r = {sx + sw - mr, sy + mt, mr, smh}};
		gfx_spr_tiled(ctx, right, dst_right, flip, mode);
	}
	if(dmw > 0 && dmh > 0 && smw > 0 && smh > 0) {
		rec_i32 dst_center    = {dx + ml, dy + mt, dmw, dmh};
		struct tex_rec center = {.t = patch.t, .r = {sx + ml, sy + mt, smw, smh}};
		gfx_spr_tiled(ctx, center, dst_center, flip, mode);
	}
}
