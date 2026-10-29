#pragma once

#include "base/types.h"

enum mem_prof_col {
	MEM_PROF_COL_PERC = 1 << 0,
	MEM_PROF_COL_BAR  = 1 << 1,
	MEM_PROF_COL_USED = 1 << 2,
	MEM_PROF_COL_PEAK = 1 << 3,
	MEM_PROF_COL_SIZE = 1 << 4,

	MEM_PROF_COL_ALL = MEM_PROF_COL_PERC | MEM_PROF_COL_BAR | MEM_PROF_COL_USED | MEM_PROF_COL_PEAK | MEM_PROF_COL_SIZE,
};
