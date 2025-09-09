#pragma once

#include "core/physics/physics.h"
#include "core/serialize/serialize.h"

void body_write(struct ser_writer *w, struct body *body);
struct body body_read(struct ser_reader *r, struct ser_value obj);
