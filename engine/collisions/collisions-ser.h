#pragma once

#include "lib/serialize/serialize.h"
#include "engine/collisions/collisions.h"

void col_shapes_write(struct ser_writer *w, struct col_shapes *shapes);
struct col_shapes col_shapes_read(struct ser_reader *r, struct ser_value obj);

void col_shape_write(struct ser_writer *w, struct col_shape *shape);
struct col_shape col_shape_read(struct ser_reader *r, struct ser_value obj);

void col_cir_write(struct ser_writer *w, struct col_cir col);
struct col_cir col_cir_read(struct ser_reader *r, struct ser_value arr);

void col_aabb_write(struct ser_writer *w, struct col_aabb col);
struct col_aabb col_aabb_read(struct ser_reader *r, struct ser_value arr);

void col_poly_write(struct ser_writer *w, struct col_poly col);
struct col_poly col_poly_read(struct ser_reader *r, struct ser_value obj);

void col_capsule_write(struct ser_writer *w, struct col_capsule col);
struct col_capsule col_capsule_read(struct ser_reader *r, struct ser_value arr);

void col_ellipsis_write(struct ser_writer *w, struct col_ellipsis value);
struct col_ellipsis col_ellipsis_read(struct ser_reader *r, struct ser_value arr);

void col_line_write(struct ser_writer *w, struct col_line value);
struct col_line col_line_read(struct ser_reader *r, struct ser_value arr);

str8
col_aabb_to_str8(struct alloc alloc, struct col_aabb col)
{
	str8 res = str8_fmt_push(alloc, "[%g,%g,%g,%g]", (double)col.min.x, (double)col.min.y, (double)col.max.x, (double)col.max.y);
	return res;
}

str8
col_cir_to_str8(struct alloc alloc, struct col_cir col)
{
	str8 res = str8_fmt_push(alloc, "{x:%g,y:%g,r:%g}", (double)col.p.x, (double)col.p.y, (double)col.r);
	return res;
}

str8
col_capsule_to_str8(struct alloc alloc, struct col_capsule col)
{
	str8 a   = col_cir_to_str8(alloc, col.a);
	str8 b   = col_cir_to_str8(alloc, col.b);
	str8 res = str8_fmt_push(alloc, "[%.*s,%.*s]", a.size, a.str, b.size, b.str);
	return res;
}

str8
col_poly_to_str8(struct alloc alloc, struct col_poly col)
{
	str8 res              = {0};
	struct str8_list list = {0};
	for(size i = 0; i < col.count; ++i) {
		struct v2 vert = col.verts[i];
		str8_list_pushf(alloc, &list, "%g", (double)vert.x);
		str8_list_pushf(alloc, &list, "%g", (double)vert.y);
	}
	struct str_join params = {.sep = str8_lit(",")};
	str8 str               = str8_list_join(alloc, &list, &params);
	res                    = str8_fmt_push(alloc, "[%.*s]", str.size, str.str);
	return res;
}
