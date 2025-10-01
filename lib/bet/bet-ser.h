#pragma once

#include "lib/bet/bet.h"
#include "lib/serialize/serialize.h"

// TODO: Move to assets
struct bet bet_load(str8 path, struct alloc alloc, struct alloc scratch);

void bet_write(struct ser_writer *w, struct bet *bet);
void bet_node_write(struct ser_writer *w, struct bet_node n);
i32 bet_read(struct ser_reader *r, struct bet *bet, struct alloc alloc);
