#pragma once

#include "collisions.h"
#include "mem.h"
#include "sys-types.h"

struct pinb_grid_cell {
	i32 x;
	i32 y;
	u32 index;
	u32 count;
	u32 tick;
};

// TODO: To avoid having to perform a hash table lookup to find out that a cell is in fact empty and is not in the table, a dense bit array with 1 bit per cell in the grid can be used s a quick pretest indicator of whether a cell is empty or not p. 288
struct pinb_grid {
	u32 cell_size;
	f32 cell_size_inv;
	u32 columns;
	u32 rows;
	i32 x_offset;
	i32 y_offset;
	struct grid_cell *cells;
	struct grid_item *items;
};

struct pinb_grid_item {
	usize index;
	struct col_shape shape;
};

void pinb_grid_gen(struct pinb_grid *grid, struct pinb_grid_item *items, usize cell_size, struct alloc alloc);
struct pinb_grid_cell pinb_grid_get(struct pinb_grid *grid, i32 x, i32 y);
