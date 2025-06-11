#pragma once

#include "mem.h"
#include "sys-log.h"
#include "sys-types.h"
#include "dbg.h"

struct marena {
#if defined DEBUG || 1
	char *name;
#endif
	void *buf_og;
	void *buf;
	usize buf_size;
	usize rem;
	char *p;
};

void marena_init(struct marena *m, void *buf, usize bufsize);
void *marena_alloc(struct marena *m, usize s);
void *marena_state(struct marena *m);
void marena_reset_to(struct marena *m, void *p);
void marena_reset(struct marena *m);
void *marena_alloc_rem(struct marena *m, usize *s);
usize marena_size_rem(struct marena *m);

// NOTE: Should this live outside?
static void *
marena_alloc_func(void *ctx, usize s)
{
	struct marena *arena = (struct marena *)ctx;
	void *mem            = marena_alloc(arena, s);
	dbg_check(mem, "marena", "Ran out of arena mem!");
#if defined DEBUG || 1
	if(arena->name != NULL) {
		log_info("marena", "%s | Alloc: %d", arena->name, (int)s);
	}
#endif
	return mem;

error:
	return NULL;
}

struct alloc
marena_allocator(struct marena *arena)
{
	struct alloc alloc = {.allocf = marena_alloc_func, .ctx = (void *)arena};
	return alloc;
}
