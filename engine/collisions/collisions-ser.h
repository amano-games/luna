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
