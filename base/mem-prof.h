#pragma once

#include "base/marena.h"
#include "base/types.h"

enum mem_prof_col {
	MEM_PROF_COL_PERC = 1 << 0,
	MEM_PROF_COL_BAR  = 1 << 1,
	MEM_PROF_COL_USED = 1 << 2,
	MEM_PROF_COL_PEAK = 1 << 3,
	MEM_PROF_COL_SIZE = 1 << 4,

	MEM_PROF_COL_ALL = MEM_PROF_COL_PERC | MEM_PROF_COL_BAR | MEM_PROF_COL_USED | MEM_PROF_COL_PEAK | MEM_PROF_COL_SIZE,
};

struct mem_prof {
	u32 cols;
	u8 sort;
};

extern struct mem_prof MEM_PROF;

static inline void
mem_prof_cols_set(u32 cols)
{
	MEM_PROF.cols = cols & MEM_PROF_COL_ALL;
}

static inline u32
mem_prof_cols_get(void)
{
	return MEM_PROF.cols;
}

static inline void
mem_prof_sort_set(enum marena_sort sort)
{
	dbg_assert(sort < MARENA_SORT_NUM_COUNT);
	MEM_PROF.sort = (u8)sort;
}

static inline enum marena_sort
mem_prof_sort_get(void)
{
	return (enum marena_sort)MEM_PROF.sort;
}
