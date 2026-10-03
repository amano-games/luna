#pragma once

#include "base/types.h"
#include "engine/gfx/gfx-txt.h"
#include "engine/gfx/gfx.h"
#include "lib/fnt/fnt.h"
#include "sys/sys.h"

static const char *SYS_MENU_VOLUME_LABELS[] = {"Mute", "Low", "Medium", "Max"};
static const f32 SYS_MENU_VOLUME_GAINS[]    = {0.f, 0.1f, 0.31622777f, 1.f};

enum sys_menu_item_type {
	SYS_MENU_ITEM_TYPE_NONE,

	SYS_MENU_ITEM_TYPE_ACTION,
	SYS_MENU_ITEM_TYPE_BOOL,
	SYS_MENU_ITEM_TYPE_OPTIONS,

	SYS_MENU_ITEM_TYPE_NUM_COUNT,
};

struct sys_menu_item {
	i32 id;
	b32 callback_pending;
	enum sys_menu_item_type type;
	str8 title;
	i32 value;
	const char **options;
	i32 options_count;
	void (*callback)(void *arg);
	void *arg;
};

struct sys_menu {
	i32 next_id;
	i32 idx;
	i32 len;
	i32 cap;
	b32 sys;
	struct sys_menu_item items[7];
};

static void
sys_menu_ini(struct sys_menu *menu)
{
	menu->next_id = 1;
	menu->cap     = ARRLEN(menu->items);
}

static void
sys_menu_callbacks(struct sys_menu *menu)
{
	i32 ids[ARRLEN(menu->items)];
	i32 len = 0;
	for(i32 i = 0; i < menu->len; ++i) {
		if(!menu->items[i].callback_pending) continue;
		menu->items[i].callback_pending = false;
		ids[len++]                      = menu->items[i].id;
	}

	for(i32 i = 0; i < len; ++i) {
		for(i32 j = 0; j < menu->len; ++j) {
			struct sys_menu_item item = menu->items[j];
			if(item.id != ids[i]) continue;
			if(item.callback) item.callback(item.arg);
			break;
		}
	}
}

static void
sys_menu_on_callback(struct sys_menu *menu)
{
	if(!menu->sys) { return; }
	sys_menu_callbacks(menu);
}

b32
sys_menu_item_next(struct sys_menu *menu)
{
	if(menu->idx + 1 >= menu->len) return false;
	++menu->idx;
	return true;
}

b32
sys_menu_item_prev(struct sys_menu *menu)
{
	if(menu->len == 0 || menu->idx == 0) return false;
	--menu->idx;
	return true;
}

void
sys_menu_item_increment(struct sys_menu *menu)
{
	if(menu->len == 0) return;
	struct sys_menu_item *item = &menu->items[menu->idx];
	if(item->type == SYS_MENU_ITEM_TYPE_BOOL && !item->value) {
		item->value            = true;
		item->callback_pending = true;
	}
	if(item->type == SYS_MENU_ITEM_TYPE_OPTIONS) {
		item->value            = (item->value + 1) % item->options_count;
		item->callback_pending = true;
	}
	sys_menu_on_callback(menu);
}

void
sys_menu_item_decrement(struct sys_menu *menu)
{
	if(menu->len == 0) return;
	struct sys_menu_item *item = &menu->items[menu->idx];
	if(item->type == SYS_MENU_ITEM_TYPE_BOOL && item->value) {
		item->value            = false;
		item->callback_pending = true;
	}
	if(item->type == SYS_MENU_ITEM_TYPE_OPTIONS) {
		item->value            = item->value == 0 ? item->options_count - 1 : item->value - 1;
		item->callback_pending = true;
	}
	sys_menu_on_callback(menu);
}

b32
sys_menu_item_confirm(struct sys_menu *menu)
{
	if(menu->len == 0) return true;

	struct sys_menu_item *item = &menu->items[menu->idx];
	switch(item->type) {
	case SYS_MENU_ITEM_TYPE_ACTION: {
		if(item->callback) item->callback(item->arg);
		return true;
	} break;
	case SYS_MENU_ITEM_TYPE_BOOL: {
		item->value            = !item->value;
		item->callback_pending = true;
	} break;
	case SYS_MENU_ITEM_TYPE_OPTIONS: {
		sys_menu_item_increment(menu);
	} break;
	default: {
	} break;
	}
	sys_menu_on_callback(menu);
	return false;
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
	i32 txt_yo                   = 2;
	i32 row_height               = min_i32(20, root.h);

	rec_i32_cut_left(&root, 10);
	rec_i32_cut_right(&root, 10);

	if(fnt.t.px1b != 0) {
		i32 x = root.x + margin;
		i32 y = cntr.y - (fnt.cell_h * 0.5f);
		fnt_mono_draw_str(ctx, fnt, str, x, y + txt_yo, 0, 0, PRIM_MODE_BLACK);
	}

	switch(type) {
	case SYS_MENU_ITEM_TYPE_OPTIONS: {
		if(fnt.t.px1b != 0) {
			str8 option = str8_cstr((char *)item.options[value]);
			i32 x       = root.x + root.w - margin - fnt_mono_size_x_px(fnt, option, 0);
			i32 y       = cntr.y - (fnt.cell_h * 0.5f);
			fnt_mono_draw_str(ctx, fnt, option, x, y + txt_yo, 0, 0, PRIM_MODE_BLACK);
		}
	} break;
	case SYS_MENU_ITEM_TYPE_BOOL: {
		i32 checkbox_w  = row_height - 6;
		i32 checkbox_ww = checkbox_w * 0.5f;
		i32 border      = 1;
		i32 x           = root.x + root.w - checkbox_w - margin;
		i32 y           = cntr.y - (checkbox_ww);
		gfx_rrec_fill(ctx, x, y, checkbox_w, checkbox_w, 2, PRIM_MODE_BLACK);
		if(!value) {
			gfx_rec_fill(ctx, x + border, y + border, checkbox_w - (border << 1), checkbox_w - (border << 1), PRIM_MODE_WHITE);
		}
		if(value) {
			gfx_cir_fill(ctx, x + checkbox_ww, y + checkbox_ww, checkbox_ww, PRIM_MODE_WHITE);
		}
	} break;
	default: {
	} break;
	}

	if(is_active) {
		gfx_rrec_fill(ctx, root.x, cntr.y - row_height / 2, root.w, row_height, 3, PRIM_MODE_INV);
	}
}

void
sys_menu_drw(const struct sys_menu *menu, struct gfx_ctx ctx, rec_i32 root, b32 is_active)
{
	if(menu->len == 0) return;
	i32 row_height = root.h / menu->len;
	for(i32 i = 0; i < menu->len; ++i) {
		rec_i32 row_layout = rec_i32_cut_top(&root, row_height);
		sys_menu_item_drw(menu->items[i], ctx, row_layout, is_active && menu->idx == i);
	}
}

static i32
sys_menu_add(
	struct sys_menu *menu,
	const char *title,
	enum sys_menu_item_type type,
	i32 value,
	void (*callback)(void *),
	void *arg)
{
	if(menu->len >= menu->cap) return 0;
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
sys_menu_add_options(struct sys_menu *menu, const char *title, const char **options, i32 count, void (*callback)(void *), void *arg)
{
	if(!options || count <= 0 || menu->len >= menu->cap) return 0;

	for(i32 i = 0; i < count; ++i) {
		if(!options[i]) return 0;
	}

	i32 id                     = sys_menu_add(menu, title, SYS_MENU_ITEM_TYPE_OPTIONS, 0, callback, arg);
	struct sys_menu_item *item = &menu->items[menu->len - 1];
	item->options              = options;
	item->options_count        = count;
	return id;
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

static i32
sys_menu_volume_idx_get(f32 volume)
{
	i32 index    = 0;
	f64 distance = 2.0;
	for(i32 i = 0; i < (i32)ARRLEN(SYS_MENU_VOLUME_GAINS); ++i) {
		f64 delta = (f64)volume - (f64)SYS_MENU_VOLUME_GAINS[i];
		if(delta < 0) delta = -delta;
		if(delta < distance) {
			distance = delta;
			index    = i;
		}
	}
	return index;
}
