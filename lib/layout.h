#pragma once

#include "mathfunc.h"

struct ui_rec {
	i32 minx, miny, maxx, maxy;
};

static inline rec_i32
ui_rec_to_rec_i32(struct ui_rec v)
{
	return (rec_i32){v.minx, v.miny, v.maxx - v.minx, v.maxy - v.miny};
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
	rect->maxy = min_i32(rect->miny, rect->maxy - a);
	return (struct ui_rec){rect->minx, rect->maxy, rect->maxx, maxy};
}

void
print_rect_ui(char *label, struct ui_rec *rect)
{
	sys_printf("%s [%d, %d, %d, %d]", label, rect->minx, rect->miny, rect->maxx, rect->maxy);
}
