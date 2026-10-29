#include "base/dbg.h"
#include "marena.h"

#define MARENA_POISONS(m) (BUILD_ASAN && !((m)->flags & MARENA_FLAG_LOCAL))

static void
marena_ini_flags(struct marena *m, void *buf, ssize buf_size, u32 flags)
{
	dbg_check_mem(buf, "Marena");

	m->buf      = buf;
	m->buf_size = buf_size;
	m->peak     = 0;
	m->flags    = flags;

	m->p = m->buf;
	if(MARENA_POISONS(m)) { MARENA_POISON(m->buf, m->buf_size); }
	marena_reset(m);

error:
	return;
}

void
marena_ini(struct marena *m, void *buf, ssize size)
{
	marena_ini_flags(m, buf, size, MARENA_FLAG_NONE);
}

void
marena_ini_local(struct marena *m, void *buf, ssize size)
{
	marena_ini_flags(m, buf, size, MARENA_FLAG_LOCAL);
}

void *
marena_alloc(struct marena *m, ssize size, ssize align)
{
	dbg_assert(align > 0);
	dbg_assert(IS_POW2(align));
	ssize mem_size    = size;
	ptrdiff_t p       = (ptrdiff_t)m->p;
	ptrdiff_t aligned = ALIGN_POW2(p, align);
	ssize padding     = aligned - p;

	if(padding > m->rem - mem_size) {
		return NULL;
	}

	m->p = (void *)(aligned + mem_size);
	m->rem -= padding + mem_size;

	// Alignment padding stays poisoned
	if(MARENA_POISONS(m)) { MARENA_UNPOISON((void *)aligned, mem_size); }

	m->peak = MAX(m->peak, m->buf_size - m->rem);

	return (void *)aligned;
}

void *
marena_state(struct marena *m)
{
	return m->p;
}

void
marena_reset_to(struct marena *m, void *p)
{
	char *prev = m->p;
	m->p       = (char *)p;
	ssize offs = ((char *)p - (char *)m->buf);

	m->rem = m->buf_size - offs;

	if(MARENA_POISONS(m) && prev > m->p) { MARENA_POISON(m->p, prev - m->p); }
}

void
marena_reset(struct marena *m)
{
	marena_reset_to(m, m->buf);
}

void *
marena_alloc_rem(struct marena *m, ssize *s)
{
	// If size is > 0 s_out = the remainder
	if(s) *s = m->rem;

	void *mem = m->p;
	if(MARENA_POISONS(m)) { MARENA_UNPOISON(mem, m->rem); }
	m->p += m->rem;
	m->rem = 0;

	m->peak = m->buf_size;

	return mem;
}

ssize
marena_size_rem(struct marena *m)
{
	return m->rem;
}

ssize
marena_size_used(struct marena *m)
{
	return m->buf_size - m->rem;
}

struct marena_tmp
marena_tmp_start(struct marena *arena)
{
	void *p               = arena->p;
	struct marena_tmp tmp = {arena, p};
	return tmp;
}

void
marena_tmp_end(struct marena_tmp tmp)
{
	marena_reset_to(tmp.arena, tmp.p);
}
