#pragma once

#include "base/context-cracking.h"
#include "base/mem.h"
#include "base/str.h"
#include "base/types.h"
#include "base/dbg.h"

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
};

struct marena_tmp {
	struct marena *arena;
	void *p;
};

#define marena_stack(var, size) \
	alignas(MEM_ALIGN_DEFAULT) u8 var##_mem[(size)] = {0}; \
	struct marena var                               = {0}; \
	marena_ini_local(&var, var##_mem, (size))

void marena_ini(struct marena *m, void *buf, ssize size);
void marena_ini_local(struct marena *m, void *buf, ssize size);
void *marena_alloc(struct marena *m, ssize size, ssize align);
void *marena_state(struct marena *m);
void marena_reset_to(struct marena *m, void *p);
void marena_reset(struct marena *m);
void *marena_alloc_rem(struct marena *m, ssize *s);
ssize marena_size_rem(struct marena *m);
ssize marena_size_used(struct marena *m);

struct marena_tmp marena_tmp_start(struct marena *arena);
void marena_tmp_end(struct marena_tmp tmp);

static inline void
marena_log_usage(const struct marena *marena, const char *tag, const char *name)
{
	ssize left  = marena_size_rem((struct marena *)marena);
	ssize total = marena->buf_size;
	ssize used  = total - left;

	char used_b[STR_BYTES_CAP], total_b[STR_BYTES_CAP], left_b[STR_BYTES_CAP], peak_b[STR_BYTES_CAP];
	str8 peak = str8_from_bytes_buf(peak_b, sizeof(peak_b), (u64)marena->peak);
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

static inline struct alloc
marena_allocator(struct marena *arena)
{
	struct alloc alloc = {.allocf = marena_alloc_func, .ctx = (void *)arena};
	return alloc;
}
