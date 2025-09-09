#pragma once

#include "core/bet/bet.h"
#include "core/serialize/serialize.h"

// TODO: Move to assets
struct bet bet_load(str8 path, struct alloc alloc, struct alloc scratch);

void bet_node_write(struct ser_writer *w, struct bet_node n);
void bet_nodes_write(struct ser_writer *w, struct bet_node *nodes, usize count);
i32 bet_read(struct ser_reader *r, struct ser_value arr, struct bet *bet, usize max_count);
