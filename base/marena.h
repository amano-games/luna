#pragma once

#include "base/context-cracking.h"
#include "base/mem.h"
#include "base/str.h"
#include "base/types.h"
#include "base/dbg.h"

#if !defined(MARENA_DEBUG)
#define MARENA_DEBUG BUILD_DEBUG
#endif

#if BUILD_ASAN && MARENA_DEBUG
#define MARENA_ASAN 1
// NOLINTBEGIN(readability-identifier-naming)
void __asan_poison_memory_region(void const volatile *addr, size_t size);
void __asan_unpoison_memory_region(void const volatile *addr, size_t size);
// NOLINTEND(readability-identifier-naming)
#define MARENA_POISON(p, s)   __asan_poison_memory_region((p), (size_t)(s))
#define MARENA_UNPOISON(p, s) __asan_unpoison_memory_region((p), (size_t)(s))
#else
#define MARENA_ASAN           0
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

#if MARENA_DEBUG
	ssize peak;
	u32 flags;
	str8 name;
	const char *file;
	i32 line;
	struct marena *parent;
#endif
};

#if MARENA_DEBUG
#define MARENA_TABLE_CAP 64
#else
#define MARENA_TABLE_CAP 1
#endif

struct marena_table {
	struct marena *arenas[MARENA_TABLE_CAP];
	ssize count;
};

extern struct marena_table MARENA_TABLE;

struct marena_tmp {
	struct marena *arena;
	void *p;
};

#define marena_stack(name, size) \
	alignas(MEM_ALIGN_DEFAULT) u8 name##_mem[(size)] = {0}; \
	struct marena name                               = {0}; \
	marena_ini_internal(&name, name##_mem, (size), MARENA_FLAG_LOCAL, str8_lit(#name), __FILE__, __LINE__)

#define marena_ini(m, buf, size)       marena_ini_internal((m), (buf), (size), 0, str8_lit(#m), __FILE__, __LINE__)
#define marena_ini_local(m, buf, size) marena_ini_internal((m), (buf), (size), MARENA_FLAG_LOCAL, str8_lit(#m), __FILE__, __LINE__)

void marena_ini_internal(struct marena *m, void *buf, ssize bufsize, u32 flags, str8 name, const char *file, i32 line);
void marena_set_name(struct marena *m, str8 name);
void marena_unregister(struct marena *m);
void *marena_alloc(struct marena *m, ssize size, ssize align);
void *marena_state(struct marena *m);
void marena_reset_to(struct marena *m, void *p);
void marena_reset(struct marena *m);
void *marena_alloc_rem(struct marena *m, ssize *s);
ssize marena_size_rem(struct marena *m);
ssize marena_size_used(struct marena *m);

static inline void
marena_log_usage(const struct marena *marena, const char *tag, const char *name)
{
	ssize left  = marena_size_rem((struct marena *)marena);
	ssize total = marena->buf_size;
	ssize used  = total - left;

	char used_b[STR_BYTES_CAP], total_b[STR_BYTES_CAP], left_b[STR_BYTES_CAP];
#if MARENA_DEBUG
	char peak_b[STR_BYTES_CAP];
	log_info(tag,
		"mem-%s| %.*s/%.*s left:%.*s peak:%.*s",
		name,
		str8_spread(str8_from_bytes_buf(used_b, sizeof(used_b), (u64)used)),
		str8_spread(str8_from_bytes_buf(total_b, sizeof(total_b), (u64)total)),
		str8_spread(str8_from_bytes_buf(left_b, sizeof(left_b), (u64)left)),
		str8_spread(str8_from_bytes_buf(peak_b, sizeof(peak_b), (u64)marena->peak)));
#else
	log_info(tag,
		"mem-%s| %.*s/%.*s left:%.*s",
		name,
		str8_spread(str8_from_bytes_buf(used_b, sizeof(used_b), (u64)used)),
		str8_spread(str8_from_bytes_buf(total_b, sizeof(total_b), (u64)total)),
		str8_spread(str8_from_bytes_buf(left_b, sizeof(left_b), (u64)left)));
#endif
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

static inline struct alloc
marena_allocator(struct marena *arena)
{
	struct alloc alloc = {.allocf = marena_alloc_func, .ctx = (void *)arena};
	return alloc;
}

struct marena_tmp marena_tmp_start(struct marena *arena);
void marena_tmp_end(struct marena_tmp tmp);
