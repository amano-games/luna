#include "ss-grid.h"

#include "arr.h"
#include "collisions.h"
#include "mathfunc.h"
#include "sys-log.h"

static inline int ss_grid_cell_col_with_shape(struct ss_grid *grid, i32 x, i32 y, struct col_shape shape);

void
ss_grid_gen(
	struct ss_grid *grid,
	struct ss_item *items,
	usize items_count,
	struct alloc alloc)
{
	f32 cell_size_inv   = 1.0f / grid->cell_size;
	grid->items         = NULL;
	grid->cell_size_inv = cell_size_inv;
	usize count         = items_count;

	i32 x1 = 0;
	i32 y1 = 0;
	i32 x2 = 0;
	i32 y2 = 0;

	// Find min/max cell cords of grid.
	for(usize i = 0; i < count; i++) {
		struct ss_item item  = items[i];
		struct col_aabb aabb = col_shape_get_bounding_box(item.shape);
		x1                   = min_i32(x1, (i32)floor_f32(aabb.min.x * cell_size_inv));
		y1                   = min_i32(y1, (i32)floor_f32(aabb.min.y * cell_size_inv));
		x2                   = max_i32(x2, (i32)floor_f32(aabb.max.x * cell_size_inv));
		y2                   = max_i32(y2, (i32)floor_f32(aabb.max.y * cell_size_inv));
	}

	i32 columns = (x2 - x1);
	i32 rows    = (y2 - y1);
	log_info("SSGrid", "Generating grid cell_size:%d columns:%d rows:%d total:%d [%d,%d] => [%d,%d]", (int)grid->cell_size, (int)columns, (int)rows, (int)(columns * rows), (int)x1, (int)y1, (int)x2, (int)y2);

	grid->columns  = columns;
	grid->rows     = rows;
	grid->x_offset = -x1;
	grid->y_offset = -y1;
	grid->cells    = arr_new(grid->cells, columns * rows, alloc);
	arr_clr(grid->cells);
	struct arr_header *header = arr_header(grid->cells);
	header->len               = arr_cap(grid->cells);
	assert(arr_len(grid->cells) == arr_cap(grid->cells));

	usize handles_count = 0;

	// Calculate how many bodies will be in each cell
	for(usize i = 0; i < count; ++i) {
		struct ss_item item  = items[i];
		struct col_aabb aabb = col_shape_get_bounding_box(item.shape);
		i32 x1               = (i32)floor_f32(aabb.min.x * cell_size_inv);
		i32 y1               = (i32)floor_f32(aabb.min.y * cell_size_inv);
		i32 x2               = (i32)floor_f32(aabb.max.x * cell_size_inv);
		i32 y2               = (i32)floor_f32(aabb.max.y * cell_size_inv);

		for(int cx = x1; cx <= x2; ++cx) {
			for(int cy = y1; cy <= y2; ++cy) {
				// NOTE: we could avoid the collision check and make this step faster
				// But this would help get less items to check when queriing the grid
				if(ss_grid_cell_col_with_shape(grid, cx, cy, item.shape)) {
					struct ss_cell *cell = ss_grid_get(grid, cx, cy);
					handles_count++;
					cell->count++;
					cell->x = cx;
					cell->y = cy;
				}
			}
		}
	}

	grid->items = arr_new(grid->items, handles_count, alloc);
	log_info("SSGrid", "Grid items:%zu cell-size:%" PRIu32 " handles:%zu", count, grid->cell_size, arr_cap(grid->items));
	for(usize i = 0; i < arr_cap(grid->items); ++i) {
		struct ss_item item = {0};
		arr_push(grid->items, item);
	}

	// Calculate indices
	for(usize i = 1; i < arr_len(grid->cells); ++i) {
		struct ss_cell *cell      = grid->cells + i;
		struct ss_cell *prev_cell = grid->cells + (i - 1);
		cell->index               = prev_cell->index + prev_cell->count;
	}

	// Reset count
	for(usize i = 0; i < arr_len(grid->cells); ++i) {
		struct ss_cell *cell = &grid->cells[i];
		grid->cells[i].count = 0;
	}

	// Store objects handles in array
	for(usize i = 0; i < count; ++i) {
		struct ss_item item  = items[i];
		struct col_aabb aabb = col_shape_get_bounding_box(item.shape);
		i32 x1               = (i32)floor_f32(aabb.min.x * cell_size_inv);
		i32 y1               = (i32)floor_f32(aabb.min.y * cell_size_inv);
		i32 x2               = (i32)floor_f32(aabb.max.x * cell_size_inv);
		i32 y2               = (i32)floor_f32(aabb.max.y * cell_size_inv);

		for(int cx = x1; cx <= x2; ++cx) {
			for(int cy = y1; cy <= y2; ++cy) {
				if(ss_grid_cell_col_with_shape(grid, cx, cy, item.shape)) {
					struct ss_cell *cell = ss_grid_get(grid, cx, cy);
					usize item_index     = cell->index + cell->count;
					assert(item_index < arr_len(grid->items));
					grid->items[item_index] = item;
					cell->count++;
				}
			}
		}
	}
}

static inline int
ss_grid_cell_col_with_shape(
	struct ss_grid *grid,
	i32 x,
	i32 y,
	struct col_shape shape)
{
	int res                   = 0;
	struct col_aabb cell_aabb = {
		.min.x = x * grid->cell_size,
		.min.y = y * grid->cell_size,
		.max.x = (x + 1) * grid->cell_size,
		.max.y = (y + 1) * grid->cell_size,
	};
	switch(shape.type) {
	case COL_TYPE_CIR: {
		res = col_circle_to_aabb(
			shape.cir.p.x,
			shape.cir.p.y,
			shape.cir.r,
			cell_aabb.min.x,
			cell_aabb.min.y,
			cell_aabb.max.x,
			cell_aabb.max.y);
	} break;
	case COL_TYPE_AABB: {
		res = col_aabb_to_aabb(
			shape.aabb.min.x,
			shape.aabb.min.y,
			shape.aabb.max.x,
			shape.aabb.max.y,
			cell_aabb.min.x,
			cell_aabb.min.y,
			cell_aabb.max.x,
			cell_aabb.max.y);
	} break;
	case COL_TYPE_POLY: {
		struct col_manifold m = {0};
		res                   = col_aabb_to_poly(
            cell_aabb.min.x,
            cell_aabb.min.y,
            cell_aabb.max.x,
            cell_aabb.max.y,
            shape.poly);
	} break;
	default: {
		struct col_aabb aabb = col_shape_get_bounding_box(shape);
		res                  = col_aabb_to_aabb(
            aabb.min.x,
            aabb.min.y,
            aabb.max.x,
            aabb.max.y,
            cell_aabb.min.x,
            cell_aabb.min.y,
            cell_aabb.max.x,
            cell_aabb.max.y);
	} break;
	}
	return res;
}

struct ss_cell *
ss_grid_get(struct ss_grid *grid, i32 x, i32 y)
{
	i32 index = 0;
	i32 mx    = x + grid->x_offset;
	i32 my    = y + grid->y_offset;
	index     = (mx * grid->columns) + my;
	assert(mx <= (i32)grid->columns);
	assert(my <= (i32)grid->rows);
	assert(index >= 0 && index < (i32)arr_len(grid->cells));
	struct ss_cell *cell = grid->cells + index;
	return cell;
}
