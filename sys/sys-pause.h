#pragma once

#include "base/mathfunc.h"
#include "base/types.h"
#include "engine/gfx/gfx-spr.h"
#include "engine/gfx/gfx.h"
#include "lib/easing-type.h"
#include "lib/easing.h"
#include "lib/tex/tex.h"
#include "sys/sys-defs.h"
#include "sys/sys-menu.h"

struct sys_pause_state {
	f32 timestamp_start;
	f32 timestamp_end;
	i32 x_offset;
	struct tex frame_tex;
	struct tex menu_tex;
	struct sys_menu menu;
};

void
sys_pause_ini(struct sys_pause_state *pause)
{
	sys_menu_ini(&pause->menu);
}

static void
sys_pause_start(
	struct sys_pause_state *pause,
	struct tex tex,
	f32 timestamp)
{
	pause->timestamp_start = timestamp;
	tex_cpy(&pause->frame_tex, &tex);
}

static void
sys_pause_end(struct sys_pause_state *pause, f32 timestamp)
{
	pause->timestamp_end = timestamp;
}

b32
sys_pause_drw(
	struct sys_pause_state *pause,
	struct gfx_ctx ctx,
	f32 timestamp)
{
	tex_clr(ctx.dst, GFX_COL_BLACK);
	i32 ani_direction        = pause->timestamp_end == 0 ? 1 : -1;
	f32 ani_timestamp        = pause->timestamp_end == 0 ? pause->timestamp_start : pause->timestamp_end;
	b32 res                  = false;
	f32 duration             = 0.2f;
	f32 elapsed              = timestamp - ani_timestamp;
	enum ease_type ease_type = ani_direction == 1 ? EASE_TYPE_QUAD_OUT : EASE_TYPE_QUAD_IN;
	f32 t                    = ease(clamp_f32(elapsed / duration, 0.0f, 1.0f), ease_type);
	t                        = ani_direction == 1 ? t : 1.0f - t;

	if(ani_direction == -1) {
		res = (timestamp - pause->timestamp_end) > duration;
	}

	{
		// Dim black
		struct tex tex     = pause->frame_tex;
		struct tex_rec src = {.t = tex, .r = {.w = tex.w, .h = tex.h}};
		gfx_spr(ctx, src, 0, 0, 0, SPR_MODE_COPY);
		ctx.pat = gfx_pattern_bayer_4x4(16 * t);
		gfx_rec_fill(ctx, 0, 0, tex.w, tex.h, PRIM_MODE_BLACK);
		ctx.pat = gfx_pattern_100();
	}

	{
		// Copy menu texture
		struct tex tex     = pause->menu_tex;
		struct tex_rec src = {.t = tex, .r = {.w = tex.w, .h = tex.h}};
		i32 x0             = SYS_DISPLAY_W;
		i32 x1             = (f32)-pause->x_offset * t;
		i32 x              = lerp(x0, x1, t);
		gfx_spr(ctx, src, x, 0, 0, SPR_MODE_COPY);
	}

	{
		// Draw menu
		i32 x0       = SYS_DISPLAY_W;
		i32 x1       = SYS_DISPLAY_W * 0.5f;
		i32 x        = lerp(x0, x1, t);
		rec_i32 root = {x, 0, SYS_DISPLAY_W * 0.5f, SYS_DISPLAY_H};
		sys_menu_drw(&pause->menu, ctx, root);
	}
	if(res) {
		pause->timestamp_end = 0;
	}
	return res;
}

static void
sys_pause_set_img(
	struct sys_pause_state *pause,
	struct tex tex,
	i32 x_offset)
{
	pause->x_offset = x_offset;
	tex_clr(pause->menu_tex, GFX_COL_CLEAR);
	if(tex.px1b == NULL) return;
	struct gfx_ctx ctx = gfx_ctx_default(pause->menu_tex);
	struct tex_rec src = {.t = tex, .r = {.w = tex.w, .h = tex.h}};
	gfx_spr(ctx, src, 0, 0, 0, SPR_MODE_COPY);
}
