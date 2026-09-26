#pragma once

#include "base/types.h"
#include "engine/gfx/gfx-txt.h"
#include "engine/gfx/gfx.h"
#include "sys/sys-input.h"
#include "sys/sys.h"

enum sys_menu_item_type {
	SYS_MENU_ITEM_TYPE_NONE,

	SYS_MENU_ITEM_TYPE_ACTION,
	SYS_MENU_ITEM_TYPE_BOOL,

	SYS_MENU_ITEM_TYPE_NUM_COUNT,
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
	struct sys_menu_item items[6];
};

static void
sys_menu_ini(struct sys_menu *menu)
{
	menu->next_id = 1;
}

b32
sys_menu_inp(struct sys_menu *menu, i32 buttons)
{
	b32 res = false;

	if(menu->len > 0) {
		struct sys_menu_item *item = menu->items + menu->idx;
		if(buttons & SYS_INP_A) {
			switch(item->type) {
			case SYS_MENU_ITEM_TYPE_ACTION: {
				res = true;
			} break;
			case SYS_MENU_ITEM_TYPE_BOOL: {
				if(item->type == SYS_MENU_ITEM_TYPE_BOOL) {
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
			if(item->type == SYS_MENU_ITEM_TYPE_BOOL) {
				item->value = true;
			}
		}
		if(buttons & SYS_INP_DPAD_L) {
			if(item->type == SYS_MENU_ITEM_TYPE_BOOL) {
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
sys_menu_item_drw(struct sys_menu_item item, struct gfx_ctx ctx, rec_i32 root, b32 is_active)
{
	v2_i32 cntr                  = rec_i32_cntr(root);
	struct fnt fnt               = sys_fnt_mono_get();
	str8 str                     = item.title;
	i32 value                    = item.value;
	enum sys_menu_item_type type = item.type;
	i32 margin                   = 4;
	rec_i32_cut_left(&root, 10);
	rec_i32_cut_right(&root, 10);
	if(fnt.t.px1b != 0) {
		i32 x = root.x + margin;
		i32 y = cntr.y - (fnt.cell_h * 0.5f);
		fnt_mono_draw_str(ctx, fnt, str, x, y, 0, 0, PRIM_MODE_BLACK);
	}
	switch(type) {
	case SYS_MENU_ITEM_TYPE_BOOL: {
		i32 checkbox_w  = 11;
		i32 checkbox_ww = checkbox_w * 0.5f;
		i32 x           = root.x + root.w - checkbox_w - margin;
		i32 y           = cntr.y - (checkbox_ww);
		gfx_rec_fill(ctx, x, y, checkbox_w, checkbox_w, PRIM_MODE_BLACK);
		if(value) {
			gfx_cir_fill(ctx, x + checkbox_ww, y + checkbox_ww, checkbox_w - 5, PRIM_MODE_WHITE);
		}
	} break;
	default: {
	} break;
	}
	if(is_active) {
		gfx_rec_fill(ctx, root.x, cntr.y - 10, root.w, 20, PRIM_MODE_INV);
	}
}

void
sys_menu_drw(const struct sys_menu *menu, struct gfx_ctx ctx, rec_i32 root)
{
	struct fnt fnt = sys_fnt_mono_get();
	gfx_rec_fill(ctx, REC_UNPACK(root), PRIM_MODE_BLACK);
	rec_i32_cut_left(&root, 3);
	gfx_rec_fill(ctx, REC_UNPACK(root), PRIM_MODE_WHITE);

	{
		i32 menu_height = 99;
		rec_i32 layout  = rec_i32_cut_top(&root, menu_height);
		i32 row_height  = menu_height / 3;

		for(ssize i = 0; i < menu->len; ++i) {
			rec_i32 row_layout        = rec_i32_cut_top(&layout, row_height);
			struct sys_menu_item item = menu->items[i];
			sys_menu_item_drw(item, ctx, row_layout, menu->idx == i);
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

static i32
sys_menu_add(struct sys_menu *menu, const char *title, enum sys_menu_item_type type, i32 value, void (*callback)(void *), void *arg)
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
sys_menu_get_value(struct sys_menu *menu, i32 id)
{
	for(i32 i = 0; i < menu->len; ++i) {
		if(menu->items[i].id == id) return menu->items[i].value;
	}
	return 0;
}

static void
sys_menu_remove(struct sys_menu *menu, i32 id)
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
sys_menu_clear(struct sys_menu *menu)
{
	mclr_array(menu->items);
	menu->len = 0;
	menu->idx = 0;
}
