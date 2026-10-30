#pragma once

#include "base/marena.h"
#include "base/mem.h"
#include "base/str.h"
#include "base/types.h"
#include "sys/sys-printf.h"

#if !defined(MEM_PROF)
#define MEM_PROF BUILD_DEBUG
#endif

#define MEM_PROF_TABLE_CAP (MEM_PROF ? 64 : 1)

struct mem_prof_info {
	struct marena *arena;
	str8 name;
	const char *file;
	i32 line;
	struct marena *parent;
};

struct mem_prof_table {
	struct mem_prof_info infos[MEM_PROF_TABLE_CAP];
	ssize count;
};

extern struct mem_prof_table MEM_PROF_TABLE;

#define MEM_PROF_NAME(s) (MEM_PROF ? str8_lit(s) : (str8){0})
#define MEM_PROF_FILE    (MEM_PROF ? __FILE__ : NULL)
#define MEM_PROF_SITE(s) MEM_PROF_NAME(s), MEM_PROF_FILE, __LINE__

#if MEM_PROF
#define marena_ini_track(m, buf, size, name) marena_ini_track_site((m), (buf), (size), MEM_PROF_SITE(name))
#else
#define marena_ini_track(m, buf, size, name) marena_ini((m), (buf), (size))
#endif

void marena_ini_track_site(struct marena *m, void *buf, ssize size, str8 name, const char *file, i32 line);
void mem_prof_track(struct marena *m, str8 name, const char *file, i32 line);
void mem_prof_untrack(struct marena *m);
void mem_prof_untrack_in_memory(void *buf, ssize size);
void mem_prof_name_set(struct marena *m, str8 name);

#define MEM_PROF_REPORT_NUM_VALUES 3

enum mem_prof_sort {
	MEM_PROF_SORT_NONE,

	MEM_PROF_SORT_DEPTH,
	MEM_PROF_SORT_USED_PERC,
	MEM_PROF_SORT_FREE_PERC,
	MEM_PROF_SORT_SIZE,

	MEM_PROF_SORT_NUM_COUNT,
};

enum mem_prof_col {
	MEM_PROF_COL_NONE,

	MEM_PROF_COL_NAME,
	MEM_PROF_COL_USED,
	MEM_PROF_COL_PEAK,
	MEM_PROF_COL_SIZE,
	MEM_PROF_COL_PERC,
	MEM_PROF_COL_BAR,

	MEM_PROF_COL_NUM_COUNT,
};

#define MEM_PROF_COL_BIT(col) (1u << (col))
#define MEM_PROF_COL_ALL      (MEM_PROF_COL_BIT(MEM_PROF_COL_NUM_COUNT) - 1)

struct mem_prof_report_entry {
	struct marena *arena;
	str8 name;
	str8 parent_name;
	const char *file;
	i32 line;
	i32 depth;
	u32 order;
	union {
		struct {
			ssize used;
			ssize peak;
			ssize size;
		};
		ssize values[MEM_PROF_REPORT_NUM_VALUES];
	};
};

struct mem_prof_report {
	enum mem_prof_sort sort;
	u32 cols;
	ssize total_used;
	ssize total_peak;
	ssize total_size;
	u32 entry_count;
	struct mem_prof_report_entry entries[MEM_PROF_TABLE_CAP];
};

struct mem_prof_report *mem_prof_report_create(struct alloc alloc, enum mem_prof_sort sort, u32 cols);
str8 mem_prof_csv(struct alloc alloc, enum mem_prof_sort sort);
b32 mem_prof_csv_save(struct alloc alloc, enum mem_prof_sort sort, str8 app_name, str8 app_org);

static inline ssize
mem_prof_bytes_div(ssize ref)
{
	i64 div = 1;
	if(ref >= MKILOBYTE(1)) { div = MKILOBYTE(1); }
	if(ref >= MMEGABYTE(1)) { div = MMEGABYTE(1); }
	if(ref >= MGIGABYTE(1)) { div = MGIGABYTE(1); }
	if(div > 1 && div < MGIGABYTE(1) && ((i64)ref * 10 + div / 2) / div >= 10240) { div *= 1024; }
	return (ssize)div;
}

static inline char
mem_prof_bytes_unit(ssize div)
{
	if(div >= MGIGABYTE(1)) { return 'G'; }
	if(div >= MMEGABYTE(1)) { return 'M'; }
	if(div >= MKILOBYTE(1)) { return 'K'; }
	return 'B';
}

static inline int
mem_prof_bytes_num(char *buf, int cap, ssize v, ssize div)
{
	if(div == 1) { return sys_snprintf(buf, cap, "%lld", (long long)v); }
	i64 tenths = ((i64)v * 10 + div / 2) / div;
	return sys_snprintf(buf, cap, "%lld.%lld", (long long)(tenths / 10), (long long)(tenths % 10));
}

// 512B, 1.5K, 2.4M
static inline str8
mem_prof_bytes_short_buf(char *buf, usize cap, ssize bytes)
{
	ssize div = mem_prof_bytes_div(bytes);
	int n     = mem_prof_bytes_num(buf, (int)cap, bytes, div);
	n += sys_snprintf(buf + n, (int)cap - n, "%c", mem_prof_bytes_unit(div));
	return str8_cstr_capped(buf, buf + n);
}

// used/total in the unit of total: 13.8/18.2M
static inline str8
mem_prof_bytes_ratio_buf(char *buf, usize cap, ssize used, ssize total)
{
	ssize div = mem_prof_bytes_div(total);
	int n     = mem_prof_bytes_num(buf, (int)cap, used, div);
	n += sys_snprintf(buf + n, (int)cap - n, "/");
	n += mem_prof_bytes_num(buf + n, (int)cap - n, total, div);
	n += sys_snprintf(buf + n, (int)cap - n, "%c", mem_prof_bytes_unit(div));
	return str8_cstr_capped(buf, buf + n);
}
