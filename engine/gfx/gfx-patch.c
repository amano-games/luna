#include "gfx-patch.h"
#include "base/dbg.h"
#include "gfx-spr.h"

void
gfx_spr_tiled(
	struct gfx_ctx ctx,
	struct tex_rec src,
	rec_i32 dst, // New: destination rect to fill (x, y, w, h)
	i32 flip,
	i32 mode,
	i32 tx,
	i32 ty)
{
	if(!src.t.px || src.r.w <= 0 || src.r.h <= 0)
		return;

	i32 tile_w = src.r.w;
	i32 tile_h = src.r.h;

	i32 x_end = dst.x + dst.w;
	i32 y_end = dst.y + dst.h;

	for(i32 y = dst.y; y < y_end; y += tile_h) {
		for(i32 x = dst.x; x < x_end; x += tile_w) {
			i32 rem_w = x_end - x;
			i32 rem_h = y_end - y;

			struct tex_rec clipped = src;

			// Crop the source rect if the tile goes out of bounds
			clipped.r.w = rem_w < tile_w ? rem_w : tile_w;
			clipped.r.h = rem_h < tile_h ? rem_h : tile_h;

			gfx_spr(ctx, clipped, x, y, flip, mode);
		}
	}
}

// BUG: margins 0 are not behaving correctly
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

	// Fixed corners
	struct tex_rec top_left     = {.t = patch.t, .r = {sx, sy, ml, mt}};
	struct tex_rec top_right    = {.t = patch.t, .r = {sx + sw - mr, sy, mr, mt}};
	struct tex_rec bottom_left  = {.t = patch.t, .r = {sx, sy + sh - mb, ml, mb}};
	struct tex_rec bottom_right = {.t = patch.t, .r = {sx + sw - mr, sy + sh - mb, mr, mb}};

	gfx_spr(ctx, top_left, dx, dy, flip, mode);
	gfx_spr(ctx, top_right, dx + dw - mr, dy, flip, mode);
	gfx_spr(ctx, bottom_left, dx, dy + dh - mb, flip, mode);
	gfx_spr(ctx, bottom_right, dx + dw - mr, dy + dh - mb, flip, mode);

	// Widths and heights of middle stretchable areas
	i32 smw = sw - ml - mr; // source middle width
	i32 smh = sh - mt - mb; // source middle height
	i32 dmw = dw - ml - mr; // destination middle width
	i32 dmh = dh - mt - mb; // destination middle height

	// Tiled edges and center
	rec_i32 dst_top    = {dx + ml, dy, dw - ml - mr, mt};
	rec_i32 dst_bottom = {dx + ml, dy + dh - mb, dw - ml - mr, mb};
	rec_i32 dst_left   = {dx, dy + mt, ml, dh - mt - mb};
	rec_i32 dst_right  = {dx + dw - mr, dy + mt, mr, dh - mt - mb};
	rec_i32 dst_center = {dx + ml, dy + mt, dw - ml - mr, dh - mt - mb};

	struct tex_rec top    = {.t = patch.t, .r = {sx + ml, sy, smw, mt}};
	struct tex_rec bottom = {.t = patch.t, .r = {sx + ml, sy + sh - mb, smw, mb}};
	struct tex_rec left   = {.t = patch.t, .r = {sx, sy + mt, ml, smh}};
	struct tex_rec right  = {.t = patch.t, .r = {sx + sw - mr, sy + mt, mr, smh}};
	struct tex_rec center = {.t = patch.t, .r = {sx + ml, sy + mt, smw, smh}};

	gfx_spr_tiled(ctx, top, dst_top, flip, mode, 0, 0);
	gfx_spr_tiled(ctx, bottom, dst_bottom, flip, mode, 0, 0);
	gfx_spr_tiled(ctx, left, dst_left, flip, mode, 0, 0);
	gfx_spr_tiled(ctx, right, dst_right, flip, mode, 0, 0);
	gfx_spr_tiled(ctx, center, dst_center, flip, mode, 0, 0);
}
