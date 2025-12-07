#pragma once

#include "engine/collisions/collisions.h"
#include "base/mem.h"
#include "base/types.h"

struct ss_cell {
	i16 x;
	i16 y;
	u16 index;
	u16 count;
	u16 tick;
};

// TODO: To avoid having to perform a hash table lookup to find out that a cell is in fact empty and is not in the table, a dense bit array with 1 bit per cell in the grid can be used s a quick pretest indicator of whether a cell is empty or not p. 288
struct ss_grid {
	u32 tick;
	i32 cell_size;
	f32 cell_size_inv;
	u32 columns;
	u32 rows;
	i32 x_offset;
	i32 y_offset;
	struct ss_cell *cells;
	struct ss_item *items;
};

struct ss_item {
	u16 index;
	u32 id; // Use as generic ID for handles
	struct col_shape shape;
};

void ss_grid_gen(struct ss_grid *grid, struct ss_item *items, ssize items_count, struct alloc alloc);
struct ss_cell *ss_grid_get(struct ss_grid *grid, i32 x, i32 y);
