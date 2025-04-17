#pragma once

#include "physics/physics.h"
#include "serialize/serialize.h"

void body_write(struct ser_writer *w, struct body body);
struct body body_read(struct ser_reader *r, struct ser_value obj);
void col_shape_write(struct ser_writer *w, struct col_shape col_shape);
struct col_shape col_shape_read(struct ser_reader *r, struct ser_value obj);
