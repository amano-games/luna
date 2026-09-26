#pragma once

#include "base/types.h"
#include "engine/gfx/gfx-spr.h"
#include "engine/gfx/gfx-txt.h"
#include "engine/gfx/gfx.h"
#include "lib/tex/tex.h"
#include "sys/sys-defs.h"
#include "sys/sys-input.h"

enum sys_menu_item_type {
	SOKOL_MENU_ITEM_TYPE_NONE,

	SOKOL_MENU_ITEM_TYPE_ACTION,
	SOKOL_MENU_ITEM_TYPE_BOOL,

	SOKOL_MENU_ITEM_TYPE_NUM_COUNT,
};

struct sys_menu_item {
	i32 id;
	enum sys_menu_item_type type;
	str8 title;
	i32 value;
	void (*callback)(void *arg);
	void *arg;
};

struct sys_menu {
	i32 next_id;
	i32 idx;
	i32 len;
	struct sys_menu_item items[3];
};

struct sys_pause_state {
	f32 timestamp;
	i32 x_offset;
	struct tex frame_tex;
	struct tex menu_tex;
	struct gfx_ctx ctx;
	struct sys_menu menu;
};

void
sys_pause_ini(struct alloc alloc, struct sys_pause_state *pause)
{
	pause->menu.next_id = 1;

	{
		struct tex tex = tex_create(alloc, SYS_DISPLAY_W, SYS_DISPLAY_H, pause->frame_tex.fmt);
		pause->ctx     = gfx_ctx_default(tex);
		dbg_check(tex.px1b, "sys-pause", "Failed to create pause gfx ctx");
	}

error:;
}

static i32
sys_pause_menu_add(struct sys_menu *menu, const char *title, enum sys_menu_item_type type, i32 value, void (*callback)(void *), void *arg)
{
	dbg_assert(menu->len < (i32)ARRLEN(menu->items));
	if(menu->len >= (i32)ARRLEN(menu->items)) return 0;
	struct sys_menu_item *item = &menu->items[menu->len++];
	*item                      = (struct sys_menu_item){
		.id       = menu->next_id++,
		.type     = type,
		.title    = str8_cstr((char *)title),
		.value    = value,
		.callback = callback,
		.arg      = arg,
	};
	return item->id;
}

static i32
sys_pause_menu_value(struct sys_menu *menu, i32 id)
{
	for(i32 i = 0; i < menu->len; ++i) {
		if(menu->items[i].id == id) return menu->items[i].value;
	}
	return 0;
}

static void
sys_pause_menu_remove(struct sys_menu *menu, i32 id)
{
	for(i32 i = 0; i < menu->len; ++i) {
		if(menu->items[i].id != id) continue;
		for(i32 j = i; j < menu->len - 1; ++j) menu->items[j] = menu->items[j + 1];
		mclr_struct(&menu->items[--menu->len]);
		if(i < menu->idx) --menu->idx;
		menu->idx = max_i32(0, min_i32(menu->idx, menu->len - 1));
		return;
	}
}

static void
sys_pause_menu_clear(struct sys_menu *menu)
{
	mclr_array(menu->items);
	menu->len = 0;
	menu->idx = 0;
}

static void
sys_pause_set_image(struct sys_pause_state *pause, struct tex tex, i32 x_offset)
{
	pause->x_offset = x_offset;
	tex_clr(pause->menu_tex, GFX_COL_CLEAR);
	if(tex.px1b == NULL) return;
	struct gfx_ctx ctx = gfx_ctx_default(pause->menu_tex);
	struct tex_rec src = {.t = tex, .r = {.w = tex.w, .h = tex.h}};
	gfx_spr(ctx, src, 0, 0, 0, SPR_MODE_COPY);
}

b32
sys_pause_inp(struct sys_menu *menu, i32 buttons)
{
	b32 res = false;

	if(menu->len > 0) {
		struct sys_menu_item *item = menu->items + menu->idx;
		if(buttons & SYS_INP_A) {
			switch(item->type) {
			case SOKOL_MENU_ITEM_TYPE_ACTION: {
				res = true;
			} break;
			case SOKOL_MENU_ITEM_TYPE_BOOL: {
				if(item->type == SOKOL_MENU_ITEM_TYPE_BOOL) {
					item->value = !item->value;
				}
			} break;
			default: {
			} break;
			}
		}
		if(buttons & SYS_INP_DPAD_U) {
			menu->idx = max_i32(menu->idx - 1, 0);
		}
		if(buttons & SYS_INP_DPAD_D) {
			menu->idx = min_i32(menu->idx + 1, menu->len - 1);
		}
		if(buttons & SYS_INP_DPAD_R) {
			if(item->type == SOKOL_MENU_ITEM_TYPE_BOOL) {
				item->value = true;
			}
		}
		if(buttons & SYS_INP_DPAD_L) {
			if(item->type == SOKOL_MENU_ITEM_TYPE_BOOL) {
				item->value = false;
			}
		}
	} else {
		if((buttons & SYS_INP_A)) {
			res = true;
		}
	}

	if((buttons & SYS_INP_B)) {
		res = true;
	}

	if(res) {
		if(menu->len > 0) {
			struct sys_menu_item *item = menu->items + menu->idx;
			if(item->callback) {
				item->callback(item->arg);
			}
		}
	}
	return res;
}

void
sys_pause_drw(struct sys_pause_state *pause)
{
	struct gfx_ctx ctx = pause->ctx;
	tex_clr(ctx.dst, GFX_COL_BLACK);

	{
		// Dim black
		struct tex tex     = pause->frame_tex;
		struct tex_rec src = {.t = tex, .r = {.w = tex.w, .h = tex.h}};
		gfx_spr(ctx, src, 0, 0, 0, SPR_MODE_COPY);
		ctx.pat = gfx_pattern_50();
		gfx_rec_fill(ctx, 0, 0, tex.w, tex.h, PRIM_MODE_BLACK);
		ctx.pat = gfx_pattern_100();
	}

	{
		struct fnt fnt       = sys_fnt_mono_get();
		struct sys_menu menu = pause->menu;
		rec_i32 root         = {SYS_DISPLAY_W * 0.5f, 0, SYS_DISPLAY_W * 0.5f, SYS_DISPLAY_H};
		gfx_rec_fill(ctx, REC_UNPACK(root), PRIM_MODE_BLACK);
		rec_i32_cut_left(&root, 3);
		gfx_rec_fill(ctx, REC_UNPACK(root), PRIM_MODE_WHITE);

		{
			i32 menu_height = 99;
			rec_i32 layout  = rec_i32_cut_top(&root, menu_height);
			i32 row_height  = menu_height / 3;
			for(ssize i = 0; i < menu.len; ++i) {
				rec_i32 row_layout           = rec_i32_cut_top(&layout, row_height);
				struct sys_menu_item item    = menu.items[i];
				str8 str                     = item.title;
				i32 value                    = item.value;
				enum sys_menu_item_type type = item.type;
				rec_i32_cut_left(&row_layout, 10);
				rec_i32_cut_right(&row_layout, 10);
				v2_i32 cntr = rec_i32_cntr(row_layout);
				if(fnt.t.px1b != 0) {
					i32 x = row_layout.x + 4;
					i32 y = cntr.y - (fnt.cell_h * 0.5f);
					fnt_mono_draw_str(ctx, fnt, str, x, y, 0, 0, PRIM_MODE_BLACK);
				}
				switch(type) {
				case SOKOL_MENU_ITEM_TYPE_BOOL: {
					i32 margin      = 4;
					i32 checkbox_w  = 11;
					i32 checkbox_ww = checkbox_w * 0.5f;
					i32 x           = row_layout.x + row_layout.w - checkbox_w - margin;
					i32 y           = cntr.y - (checkbox_ww);
					gfx_rec_fill(ctx, x, y, checkbox_w, checkbox_w, PRIM_MODE_BLACK);
					if(value) {
						gfx_cir_fill(ctx, x + checkbox_ww, y + checkbox_ww, checkbox_w - 5, PRIM_MODE_WHITE);
					}
				} break;
				default: {
				} break;
				}
				if(menu.idx == i) {
					gfx_rec_fill(ctx, row_layout.x, cntr.y - 10, row_layout.w, 20, PRIM_MODE_INV);
				}
			}
		}
		{
			rec_i32 layout = rec_i32_cut_top(&root, 2);
			rec_i32_cut_left(&layout, 10);
			rec_i32_cut_right(&layout, 10);
			gfx_lin(ctx, layout.x, layout.y, layout.x + layout.w, layout.y, PRIM_MODE_BLACK);
			gfx_lin(ctx, layout.x, layout.y + 1, layout.x + layout.w, layout.y + 1, PRIM_MODE_BLACK);
		}
	}
	{
		struct tex tex     = pause->menu_tex;
		struct tex_rec src = {.t = tex, .r = {.w = tex.w, .h = tex.h}};
		gfx_spr(ctx, src, -pause->x_offset, 0, 0, SPR_MODE_COPY);
	}
}
