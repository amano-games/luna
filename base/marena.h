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

void marena_init(struct marena *m, void *buf, ssize bufsize);
void *marena_alloc(struct marena *m, ssize s);
void *marena_state(struct marena *m);
void marena_reset_to(struct marena *m, void *p);
void marena_reset(struct marena *m);
void *marena_alloc_rem(struct marena *m, ssize *s);
ssize marena_size_rem(struct marena *m);

// NOTE: Should this live outside?
static void *
marena_alloc_func(void *ctx, ssize s)
{
	struct marena *arena = (struct marena *)ctx;
	void *mem            = marena_alloc(arena, s);
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
