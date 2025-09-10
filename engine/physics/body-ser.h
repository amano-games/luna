#pragma once

#include "engine/physics/physics.h"
#include "lib/serialize/serialize.h"

void body_write(struct ser_writer *w, struct body *body);
struct body body_read(struct ser_reader *r, struct ser_value obj);
