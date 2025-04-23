#include "pinb-grid.h"

#include "arr.h"
#include "collisions.h"
#include "mathfunc.h"

void
pinb_grid_gen(struct pinb_grid *grid, struct pinb_grid_item *items, usize cell_size, struct alloc alloc)
{
	f32 cell_size_inv   = 1.0f / cell_size;
	grid->items         = NULL;
	grid->cell_size     = cell_size;
	grid->cell_size_inv = cell_size_inv;
	usize start         = 0;
	usize count         = arr_len(items);

	i32 x1 = 0;
	i32 y1 = 0;
	i32 x2 = 0;
	i32 y2 = 0;

	for(usize i = 0; i < count; i++) {
		struct pinb_grid_item item = items[i];
		struct col_aabb aabb       = col_shape_get_bounding_box(item.shape);
		x1                         = min_i32(x1, (i32)floor_f32(aabb.min.x * cell_size_inv));
		y1                         = min_i32(y1, (i32)floor_f32(aabb.min.y * cell_size_inv));
		x2                         = max_i32(x2, (i32)floor_f32(aabb.max.x * cell_size_inv));
		y2                         = max_i32(y2, (i32)floor_f32(aabb.max.y * cell_size_inv));
	}

	i32 columns = (x2 - x1);
	i32 rows    = (y2 - y1);
	log_info("World", "Generating grid cell_size: %d columns: %d rows: %d total: %d [%d,%d] => [%d,%d]", (int)cell_size, (int)columns, (int)rows, (int)(columns * rows), (int)x1, (int)y1, (int)x2, (int)y2);

	grid->columns  = columns;
	grid->rows     = rows;
	grid->x_offset = -x1;
	grid->y_offset = -y1;
	grid->cells    = arr_ini(columns * rows, sizeof(*grid->cells), alloc);
	arr_zero(grid->cells, sizeof(*grid->cells));
	struct arr_header *header = arr_header(grid->cells);
	header->len               = arr_cap(grid->cells);
	assert(arr_len(grid->cells) == arr_cap(grid->cells));

	usize handles_count = 0;

	// Calculate how many bodies will be in each cell
	for(usize i = 0; i < count; ++i) {
		usize world_index      = start + i;
		struct grid_item *item = &items[i];
		struct entity *entity  = world_get_entity(world, item->handle);
		struct col_aabb *aabb  = &item->aabb;
		i32 x1                 = (i32)floorf(aabb->min.x * cell_size_inv);
		i32 y1                 = (i32)floorf(aabb->min.y * cell_size_inv);
		i32 x2                 = (i32)floorf(aabb->max.x * cell_size_inv);
		i32 y2                 = (i32)floorf(aabb->max.y * cell_size_inv);

		for(int cx = x1; cx <= x2; ++cx) {
			for(int cy = y1; cy <= y2; ++cy) {
				struct grid_cell *cell = pinb_grid_get(grid, cx, cy);
				handles_count++;
				cell->count++;
				cell->x = cx;
				cell->y = cy;
			}
		}
	}

	grid->items = arr_ini(handles_count, sizeof(*grid->items), alloc);
	log_info("World", "Grid entities:%zu cell-size:%" PRIu32 " handles:%zu items:%zu", arr_len(items), grid->cell_size, arr_cap(grid->items), arr_len(items));
	for(usize i = 0; i < arr_cap(grid->items); ++i) {
		struct grid_item item = {0};
		arr_push(grid->items, item);
	}

	// Calculate indices
	for(usize i = 1; i < arr_len(grid->cells); ++i) {
		struct grid_cell *cell      = &grid->cells[i];
		struct grid_cell *prev_cell = &grid->cells[i - 1];

		cell->index = prev_cell->index + prev_cell->count;
	}

	// Reset count
	for(usize i = 0; i < arr_len(grid->cells); ++i) {
		struct grid_cell *cell = &grid->cells[i];
		grid->cells[i].count   = 0;
	}

	// Store objects handles in array
	for(usize i = 0; i < count; ++i) {
		usize world_index     = start + i;
		struct grid_item item = items[i];

		struct col_aabb aabb = col_shape_get_bounding_box(item.shape);
		i32 x1               = (i32)floorf(aabb.min.x * cell_size_inv);
		i32 y1               = (i32)floorf(aabb.min.y * cell_size_inv);
		i32 x2               = (i32)floorf(aabb.max.x * cell_size_inv);
		i32 y2               = (i32)floorf(aabb.max.y * cell_size_inv);

		for(int cx = x1; cx <= x2; ++cx) {
			for(int cy = y1; cy <= y2; ++cy) {
				// sys_printf("%d,%d", cx, cy);
				struct grid_cell *cell = world_grid_get(grid, cx, cy);
				usize item_index       = cell->index + cell->count;

				assert(item_index < arr_len(grid->items));

				grid->items[item_index] = item;
				cell->count++;
			}
		}
	}
}
