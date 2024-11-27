#pragma once

#include "mathfunc.h"

enum ui_cut_side {
	UI_CUT_NONE,
	UI_CUT_LEFT,
	UI_CUT_RIGHT,
	UI_CUT_TOP,
	UI_CUT_BOTTOM,
};

struct ui_rec {
	i32 minx, miny, maxx, maxy;
};

struct ui_cut {
	struct ui_rec *rect;
	enum ui_cut_side side;
};

static inline struct ui_rec
ui_rec_from_rec(i32 x, i32 y, i32 w, i32 h)
{
	return (struct ui_rec){x, y, x + w, y + h};
}

static inline struct ui_rec
ui_rec_from_rec_pivot(i32 x, i32 y, i32 w, i32 h, v2 pivot)
{
	i32 nx = x - (w * pivot.x);
	i32 ny = y - (h * pivot.y);
	return (struct ui_rec){nx, ny, nx + w, ny + h};
}

static inline rec_i32
ui_rec_to_rec_i32(struct ui_rec v)
{
	return (rec_i32){v.minx, v.miny, v.maxx - v.minx, v.maxy - v.miny};
}

struct ui_cut
ui_cut(struct ui_rec *rect, enum ui_cut_side side)
{
	return (struct ui_cut){
		.rect = rect,
		.side = side};
}

struct ui_rec
ui_cut_left(struct ui_rec *rect, i32 a)
{
	i32 minx   = rect->minx;
	rect->minx = min_i32(rect->maxx, rect->minx + a);
	return (struct ui_rec){minx, rect->miny, rect->minx, rect->maxy};
}

struct ui_rec
ui_cut_right(struct ui_rec *rect, i32 a)
{
	i32 maxx   = rect->maxx;
	rect->maxx = max_i32(rect->minx, rect->maxx - a);
	return (struct ui_rec){rect->maxx, rect->miny, maxx, rect->maxy};
}

struct ui_rec
ui_cut_top(struct ui_rec *rect, i32 a)
{
	i32 miny   = rect->miny;
	rect->miny = min_i32(rect->maxy, rect->miny + a);
	return (struct ui_rec){rect->minx, miny, rect->maxx, rect->miny};
}

struct ui_rec
ui_cut_bottom(struct ui_rec *rect, i32 a)
{
	i32 maxy   = rect->maxy;
	rect->maxy = max_i32(rect->miny, rect->maxy - a);
	return (struct ui_rec){rect->minx, rect->maxy, rect->maxx, maxy};
}

struct ui_rec
ui_rect_cut(struct ui_cut ui_cut, i32 a)
{
	switch(ui_cut.side) {
	case UI_CUT_LEFT: return ui_cut_left(ui_cut.rect, a);
	case UI_CUT_RIGHT: return ui_cut_right(ui_cut.rect, a);
	case UI_CUT_TOP: return ui_cut_top(ui_cut.rect, a);
	case UI_CUT_BOTTOM: return ui_cut_bottom(ui_cut.rect, a);
	default: {
		BAD_PATH;
		return (struct ui_rec){0};
	}
	}
}

struct ui_rec
ui_get_left(const struct ui_rec *rect, i32 a)
{
	i32 maxx = min_i32(rect->maxx, rect->minx + a);
	return (struct ui_rec){rect->minx, rect->miny, maxx, rect->maxy};
}

struct ui_rec
ui_get_right(const struct ui_rec *rect, i32 a)
{
	i32 minx = max_i32(rect->minx, rect->maxx - a);
	return (struct ui_rec){minx, rect->miny, rect->maxx, rect->maxy};
}

struct ui_rec
ui_get_top(const struct ui_rec *rect, i32 a)
{
	i32 maxy = min_i32(rect->maxy, rect->miny + a);
	return (struct ui_rec){rect->minx, rect->miny, rect->maxy, maxy};
}

struct ui_rec
ui_get_bottom(const struct ui_rec *rect, i32 a)
{
	i32 miny = max_i32(rect->miny, rect->maxy - a);
	return (struct ui_rec){rect->minx, miny, rect->maxy, rect->maxy};
}

void
print_rect_ui(char *label, struct ui_rec *rect)
{
	sys_printf("%s [%d, %d, %d, %d]", label, rect->minx, rect->miny, rect->maxx, rect->maxy);
}
