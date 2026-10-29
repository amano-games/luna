#pragma once

#include "base/context-cracking.h"
#include "base/mem.h"
#include "base/str.h"
#include "base/types.h"
#include "base/dbg.h"
#include "sys/sys-printf.h"

#if !defined(MARENA_TRACK)
#define MARENA_TRACK BUILD_DEBUG
#endif

#if BUILD_ASAN
// NOLINTBEGIN(readability-identifier-naming)
void __asan_poison_memory_region(void const volatile *addr, size_t size);
void __asan_unpoison_memory_region(void const volatile *addr, size_t size);
// NOLINTEND(readability-identifier-naming)
#define MARENA_POISON(p, s)   __asan_poison_memory_region((p), (size_t)(s))
#define MARENA_UNPOISON(p, s) __asan_unpoison_memory_region((p), (size_t)(s))
#else
#define MARENA_POISON(p, s)   ((void)(p), (void)(s))
#define MARENA_UNPOISON(p, s) ((void)(p), (void)(s))
#endif

enum marena_flag {
	MARENA_FLAG_NONE,

	MARENA_FLAG_LOCAL = 1 << 0,
};

struct marena {
	char *buf;
	ssize buf_size;
	ssize rem;
	char *p;

	ssize peak;
	u32 flags;
	str8 name;
	const char *file;
	i32 line;
	struct marena *parent;
};

#define MARENA_TABLE_CAP (MARENA_TRACK ? 64 : 1)

struct marena_table {
	struct marena *arenas[MARENA_TABLE_CAP];
	ssize count;
};

extern struct marena_table MARENA_TABLE;

#define MARENA_REPORT_NUM_VALUES 3

enum marena_sort {
	MARENA_SORT_DEPTH,
	MARENA_SORT_USED_PERC,
	MARENA_SORT_FREE_PERC,
	MARENA_SORT_SIZE,

	MARENA_SORT_NUM_COUNT,
};

struct marena_report_entry {
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
		ssize values[MARENA_REPORT_NUM_VALUES];
	};
};

struct marena_report {
	ssize total_used;
	ssize total_peak;
	ssize total_size;
	u32 entry_count;
	struct marena_report_entry entries[MARENA_TABLE_CAP];
};

struct marena_tmp {
	struct marena *arena;
	void *p;
};

#define MARENA_NAME(s) (MARENA_TRACK ? str8_lit(s) : (str8){0})
#define MARENA_FILE    (MARENA_TRACK ? __FILE__ : NULL)
#define MARENA_SITE(s) MARENA_NAME(s), MARENA_FILE, __LINE__

#define marena_stack(var, size) \
	alignas(MEM_ALIGN_DEFAULT) u8 var##_mem[(size)] = {0}; \
	struct marena var                               = {0}; \
	marena_ini_internal(&var, var##_mem, (size), MARENA_FLAG_LOCAL, (str8){0}, MARENA_FILE, __LINE__)

#define marena_ini(m, buf, size, name) marena_ini_internal((m), (buf), (size), 0, MARENA_SITE(name))
#define marena_ini_local(m, buf, size) marena_ini_internal((m), (buf), (size), MARENA_FLAG_LOCAL, (str8){0}, MARENA_FILE, __LINE__)

void marena_ini_internal(struct marena *m, void *buf, ssize bufsize, u32 flags, str8 name, const char *file, i32 line);
void marena_unregister(struct marena *m);
void marena_unregister_in_memory(void *buf, ssize size);
void *marena_alloc(struct marena *m, ssize size, ssize align);
void *marena_state(struct marena *m);
void marena_reset_to(struct marena *m, void *p);
void marena_reset(struct marena *m);
void *marena_alloc_rem(struct marena *m, ssize *s);
ssize marena_size_rem(struct marena *m);
ssize marena_size_used(struct marena *m);

struct marena_tmp marena_tmp_start(struct marena *arena);
void marena_tmp_end(struct marena_tmp tmp);

struct marena_report *marena_report_create(struct alloc alloc, enum marena_sort sort);
str8 marena_csv(struct alloc alloc);
b32 marena_csv_save(struct alloc alloc, str8 app_name, str8 app_org);

static inline void
marena_set_name(struct marena *m, str8 name)
{
	if(MARENA_TRACK) { m->name = name; }
}

static inline void
marena_log_usage(const struct marena *marena, const char *tag, const char *name)
{
	ssize left  = marena_size_rem((struct marena *)marena);
	ssize total = marena->buf_size;
	ssize used  = total - left;

	char used_b[STR_BYTES_CAP], total_b[STR_BYTES_CAP], left_b[STR_BYTES_CAP], peak_b[STR_BYTES_CAP];
	str8 peak = MARENA_TRACK ? str8_from_bytes_buf(peak_b, sizeof(peak_b), (u64)marena->peak) : str8_lit("-");
	log_info(tag,
		"mem-%s| %.*s/%.*s left:%.*s peak:%.*s",
		name,
		str8_spread(str8_from_bytes_buf(used_b, sizeof(used_b), (u64)used)),
		str8_spread(str8_from_bytes_buf(total_b, sizeof(total_b), (u64)total)),
		str8_spread(str8_from_bytes_buf(left_b, sizeof(left_b), (u64)left)),
		str8_spread(peak));
}

#define MARENA_LOG_USAGE(marena, tag) \
	marena_log_usage((marena), (tag), #marena)

static void *
marena_alloc_func(void *ctx, ssize size, ssize align)
{
	struct marena *arena = (struct marena *)ctx;
	void *mem            = marena_alloc(arena, size, align);
	dbg_check(mem, "marena", "Ran out of arena mem!");
	return mem;

error:
	return NULL;
}

static inline ssize
marena_bytes_div(ssize ref)
{
	i64 div = 1;
	if(ref >= MKILOBYTE(1)) { div = MKILOBYTE(1); }
	if(ref >= MMEGABYTE(1)) { div = MMEGABYTE(1); }
	if(ref >= MGIGABYTE(1)) { div = MGIGABYTE(1); }
	if(div > 1 && div < MGIGABYTE(1) && ((i64)ref * 10 + div / 2) / div >= 10240) { div *= 1024; }
	return (ssize)div;
}

static inline char
marena_bytes_unit(ssize div)
{
	if(div >= MGIGABYTE(1)) { return 'G'; }
	if(div >= MMEGABYTE(1)) { return 'M'; }
	if(div >= MKILOBYTE(1)) { return 'K'; }
	return 'B';
}

static inline int
marena_bytes_num(char *buf, int cap, ssize v, ssize div)
{
	if(div == 1) { return sys_snprintf(buf, cap, "%lld", (long long)v); }
	i64 tenths = ((i64)v * 10 + div / 2) / div;
	return sys_snprintf(buf, cap, "%lld.%lld", (long long)(tenths / 10), (long long)(tenths % 10));
}

// 512B, 1.5K, 2.4M
static inline str8
marena_bytes_short_buf(char *buf, usize cap, ssize bytes)
{
	ssize div = marena_bytes_div(bytes);
	int n     = marena_bytes_num(buf, (int)cap, bytes, div);
	n += sys_snprintf(buf + n, (int)cap - n, "%c", marena_bytes_unit(div));
	return str8_cstr_capped(buf, buf + n);
}

// used/total in the unit of total: 13.8/18.2M
static inline str8
marena_bytes_ratio_buf(char *buf, usize cap, ssize used, ssize total)
{
	ssize div = marena_bytes_div(total);
	int n     = marena_bytes_num(buf, (int)cap, used, div);
	n += sys_snprintf(buf + n, (int)cap - n, "/");
	n += marena_bytes_num(buf + n, (int)cap - n, total, div);
	n += sys_snprintf(buf + n, (int)cap - n, "%c", marena_bytes_unit(div));
	return str8_cstr_capped(buf, buf + n);
}

static inline struct alloc
marena_allocator(struct marena *arena)
{
	struct alloc alloc = {.allocf = marena_alloc_func, .ctx = (void *)arena};
	return alloc;
}
