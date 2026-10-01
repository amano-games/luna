#pragma once

#include "base/mathfunc.h"
#include "base/types.h"
#include "engine/gfx/gfx-spr.h"
#include "engine/gfx/gfx.h"
#include "lib/easing-type.h"
#include "lib/easing.h"
#include "lib/tex/tex.h"
#include "sys/sys.h"
#include "sys/sys-input.h"
#include "sys/sys-menu.h"

enum SYS_PAUSE_MENU_TYPE {
	SYS_PAUSE_MENU_TYPE_APP,
	SYS_PAUSE_MENU_TYPE_SYS,

	SYS_PAUSE_MENU_TYPE_NUM_COUNT,
};

struct sys_pause_state {
	f32 timestamp_start;
	f32 timestamp_end;
	i32 x_offset;
	struct tex frame_tex;
	struct tex menu_tex;
	struct sys_menu menus[SYS_PAUSE_MENU_TYPE_NUM_COUNT];
	i32 active_menu;
};

void
sys_pause_ini(struct sys_pause_state *pause)
{
	for(ssize i = 0; i < (ssize)ARRLEN(pause->menus); i++) {
		sys_menu_ini(&pause->menus[i]);
	}
	pause->menus[SYS_PAUSE_MENU_TYPE_APP].cap = 3;
	pause->menus[SYS_PAUSE_MENU_TYPE_SYS].sys = true;
	pause->active_menu                        = SYS_PAUSE_MENU_TYPE_APP;
}

static struct sys_menu *
sys_pause_active_menu(struct sys_pause_state *pause)
{
	struct sys_menu *res = &pause->menus[SYS_PAUSE_MENU_TYPE_APP];

	for(i32 i = pause->active_menu; i < (i32)ARRLEN(pause->menus); ++i) {
		if(pause->menus[i].len == 0) continue;
		pause->active_menu = i;
		res                = &pause->menus[i];
		goto cleanup;
	}
	for(i32 i = pause->active_menu - 1; i >= 0; --i) {
		if(pause->menus[i].len == 0) continue;
		pause->active_menu = i;
		res                = &pause->menus[i];
		goto cleanup;
	}

	pause->active_menu = SYS_PAUSE_MENU_TYPE_APP;
cleanup:;
	return res;
}

b32
sys_pause_inp(struct sys_pause_state *pause, i32 buttons)
{
	if(pause->timestamp_end != 0) return false;
	if(buttons & SYS_INP_B) return true;
	struct sys_menu *menu = sys_pause_active_menu(pause);
	i32 step              = 0;
	b32 res               = false;

	if(buttons & SYS_INP_DPAD_R) {
		sys_menu_item_increment(menu);
	} else if(buttons & SYS_INP_DPAD_L) {
		sys_menu_item_decrement(menu);
	} else if(buttons & SYS_INP_DPAD_D) {
		if(!sys_menu_item_next(menu)) step = 1;
	} else if(buttons & SYS_INP_DPAD_U) {
		if(!sys_menu_item_prev(menu)) step = -1;
	} else if(buttons & SYS_INP_A) {
		res = sys_menu_item_confirm(menu);
	}

	if(step) {
		for(i32 i = pause->active_menu + step; i >= 0 && i < (i32)ARRLEN(pause->menus); i += step) {
			if(pause->menus[i].len == 0) continue;
			pause->active_menu  = i;
			pause->menus[i].idx = step > 0 ? 0 : pause->menus[i].len - 1;
			break;
		}
	}

	sys_pause_active_menu(pause);
	return res;
}

static void
sys_pause_start(
	struct sys_pause_state *pause,
	struct tex tex,
	f32 timestamp)
{
	pause->timestamp_start = timestamp;
	pause->timestamp_end   = 0;
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
	v2_i32 sys_resolution = sys_resolution_get();
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
		f32 alpha          = min_f32(t, 0.7f);
		gfx_spr(ctx, src, 0, 0, 0, SPR_MODE_COPY);
		ctx.pat = gfx_pattern_bayer_4x4(16 * alpha);
		gfx_rec_fill(ctx, 0, 0, tex.w, tex.h, PRIM_MODE_BLACK);
		ctx.pat = gfx_pattern_100();
	}

	{
		// Copy menu texture
		struct tex tex     = pause->menu_tex;
		struct tex_rec src = {.t = tex, .r = {.w = tex.w, .h = tex.h}};
		i32 x0             = sys_resolution.x;
		i32 x1             = (f32)-pause->x_offset * t;
		i32 x              = lerp(x0, x1, t);
		gfx_spr(ctx, src, x, 0, 0, SPR_MODE_COPY);
	}

	{
		// Draw the panel and menus in array order, with dividers between them.
		i32 x0       = sys_resolution.x;
		i32 x1       = sys_resolution.x * 0.5f;
		i32 x        = lerp(x0, x1, t);
		rec_i32 root = {x, 0, sys_resolution.x * 0.5f, sys_resolution.y};
		gfx_rec_fill(ctx, REC_UNPACK(root), PRIM_MODE_BLACK);
		rec_i32_cut_left(&root, 3);
		gfx_rec_fill(ctx, REC_UNPACK(root), PRIM_MODE_WHITE);
		root = rec_i32_cut_bottom(&root, SYS_PD_DISPLAY_H);

		sys_pause_active_menu(pause);
		i32 rows       = 0;
		i32 menu_count = (i32)ARRLEN(pause->menus);
		for(i32 i = 0; i < menu_count; ++i) rows += pause->menus[i].len;
		i32 divider_height   = 2;
		i32 available_height = max_i32(0, root.h - divider_height * (menu_count - 1));
		i32 row_height       = rows > 0 ? min_i32(33, available_height / rows) : 33;
		for(i32 i = 0; i < menu_count; ++i) {
			struct sys_menu *menu = &pause->menus[i];
			rec_i32 layout        = rec_i32_cut_top(&root, menu->len * row_height);
			sys_menu_drw(menu, ctx, layout, pause->active_menu == i);
			if(i == menu_count - 1) continue;

			rec_i32 divider = rec_i32_cut_top(&root, divider_height);
			rec_i32_cut_left(&divider, 10);
			rec_i32_cut_right(&divider, 10);
			gfx_lin(ctx, divider.x, divider.y, divider.x + divider.w, divider.y, PRIM_MODE_BLACK);
			gfx_lin(ctx, divider.x, divider.y + 1, divider.x + divider.w, divider.y + 1, PRIM_MODE_BLACK);
		}
	}
	if(res) {
		pause->timestamp_end = 0;
		for(i32 i = 0; i < (i32)ARRLEN(pause->menus); ++i) {
			sys_menu_callbacks(&pause->menus[i]);
		}
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
	gfx_spr(ctx, src, 0, pause->menu_tex.h - tex.h, 0, SPR_MODE_COPY);
}
