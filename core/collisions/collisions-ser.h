#pragma once

#include "serialize/serialize.h"
#include "collisions.h"

void col_shapes_write(struct ser_writer *w, struct col_shapes shapes);
void col_shape_write(struct ser_writer *w, struct col_shape shape);
void col_cir_write(struct ser_writer *w, struct col_cir col);
void col_aabb_write(struct ser_writer *w, struct col_aabb col);
void col_poly_write(struct ser_writer *w, struct col_poly col);
void col_capsule_write(struct ser_writer *w, struct col_capsule col);

struct col_shapes col_shapes_read(struct ser_reader *r, struct ser_value obj);
struct col_shape col_shape_read(struct ser_reader *r, struct ser_value obj);
struct v2 col_v2_read(struct ser_reader *r, struct ser_value arr);
struct col_cir col_cir_read(struct ser_reader *r, struct ser_value arr);
struct col_aabb col_aabb_read(struct ser_reader *r, struct ser_value arr);
struct col_poly col_poly_read(struct ser_reader *r, struct ser_value obj);
struct col_capsule col_capsule_read(struct ser_reader *r, struct ser_value arr);
