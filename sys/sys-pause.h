#pragma once

#include "base/types.h"
#include "engine/gfx/gfx-spr.h"
#include "engine/gfx/gfx.h"
#include "lib/easing-type.h"
#include "lib/easing.h"
#include "lib/tex/tex.h"
#include "sys/sys-defs.h"
#include "sys/sys-menu.h"

struct sys_pause_state {
	f32 timestamp;
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
	pause->timestamp = timestamp;
	tex_cpy(&pause->frame_tex, &tex);
}

void
sys_pause_drw(
	struct sys_pause_state *pause,
	struct gfx_ctx` ctx,
	f32 timestamp)
{
	tex_clr(ctx.dst, GFX_COL_BLACK);

	{
		// Dim black
		struct tex tex     = pause->frame_tex;
		struct tex_rec src = {.t = tex, .r = {.w = tex.w, .h = tex.h}};
		gfx_spr(ctx, src, 0, 0, 0, SPR_MODE_COPY);
		f32 duration = 0.3f;
		f32 elapsed  = timestamp - pause->timestamp;
		f32 t        = ease(clamp_f32(elapsed / duration, 0.0f, 1.0f), EASE_TYPE_QUART_OUT);
		ctx.pat      = gfx_pattern_bayer_4x4(16 * t);
		gfx_rec_fill(ctx, 0, 0, tex.w, tex.h, PRIM_MODE_BLACK);
		ctx.pat = gfx_pattern_100();
	}

	{
		struct tex tex     = pause->menu_tex;
		struct tex_rec src = {.t = tex, .r = {.w = tex.w, .h = tex.h}};
		f32 duration       = 0.3f;
		f32 elapsed        = timestamp - (pause->timestamp);
		f32 t              = ease(clamp_f32(elapsed / duration, 0.0f, 1.0f), EASE_TYPE_QUART_OUT);
		i32 x              = (f32)-pause->x_offset * t;
		gfx_spr(ctx, src, x, 0, 0, SPR_MODE_COPY);
	}

	{
		f32 duration = 0.2f;
		f32 elapsed  = timestamp - (pause->timestamp);
		f32 t        = ease(clamp_f32(elapsed / duration, 0.0f, 1.0f), EASE_TYPE_QUART_OUT);
		i32 tx       = SYS_DISPLAY_W - ((f32)(SYS_DISPLAY_W * 0.5f) * t);
		rec_i32 root = {tx, 0, SYS_DISPLAY_W * 0.5f, SYS_DISPLAY_H};
		sys_menu_drw(&pause->menu, ctx, root);
	}
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
