#pragma once

#include "base/context-cracking.h"
#include "base/mem.h"
#include "base/str.h"
#include "base/types.h"
#include "base/dbg.h"

#if !defined(MARENA_DEBUG)
#define MARENA_DEBUG BUILD_DEBUG
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
	const char *name;
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
	marena_ini_internal(&name, name##_mem, (size), MARENA_FLAG_LOCAL, #name, __FILE__, __LINE__)

#define marena_init(m, buf, size)       marena_ini_internal((m), (buf), (size), 0, #m, __FILE__, __LINE__)
#define marena_init_local(m, buf, size) marena_ini_internal((m), (buf), (size), MARENA_FLAG_LOCAL, #m, __FILE__, __LINE__)

void marena_ini_internal(struct marena *m, void *buf, ssize bufsize, u32 flags, const char *name, const char *file, i32 line);
void marena_set_name(struct marena *m, const char *name);
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
