#pragma once

#include "base/mem.h"
#include "base/types.h"
#include "base/dbg.h"

struct marena {
	char *buf;
	ssize buf_size;
	ssize rem;
	char *p;
};

struct marena_tmp {
	struct marena *arena;
	void *p;
};

#define marena_stack(name, size) \
	alignas(MEM_ALIGN_DEFAULT) u8 name##_mem[(size)] = {0}; \
	struct marena name                               = {0}; \
	marena_init(&name, name##_mem, (size))

void marena_init(struct marena *m, void *buf, ssize bufsize);
void *marena_alloc(struct marena *m, ssize size, ssize align);
void *marena_state(struct marena *m);
void marena_reset_to(struct marena *m, void *p);
void marena_reset(struct marena *m);
void *marena_alloc_rem(struct marena *m, ssize *s);
ssize marena_size_rem(struct marena *m);

static inline void
marena_log_usage(const struct marena *marena, const char *tag, const char *name)
{
	ssize left  = marena_size_rem((struct marena *)marena);
	ssize total = marena->buf_size;
	ssize used  = total - left;

	log_info(tag,
		"mem-%s| %_$u/%_$u left:%_$u",
		name,
		(uint)used,
		(uint)total,
		(uint)left);
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
