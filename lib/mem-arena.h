#pragma once

#include "mem.h"
#include "sys-assert.h"
#include "sys-log.h"
#include "sys-types.h"

struct marena {
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
marena_alloc_ctx(void *ctx, usize s)
{
	struct marena *arena = (struct marena *)ctx;
	void *mem            = marena_alloc(arena, s);

	if(!mem) {
		sys_printf("Ran out of arena mem!\n");
		BAD_PATH
	}

	return mem;
}

struct alloc
marena_allocator(struct marena *arena)
{
	struct alloc alloc = {marena_alloc_ctx, (void *)arena};
	return alloc;
}
