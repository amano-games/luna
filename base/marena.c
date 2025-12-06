#include "base/mem.h"
#include "base/dbg.h"
#include "marena.h"

void
marena_init(struct marena *m, void *buf, ssize bufsize)
{
	dbg_check_mem(buf, "Marena");
	mspan sp    = {buf, bufsize};
	sp          = mspan_align(sp);
	m->buf      = sp.p;
	m->buf_size = sp.size;

	marena_reset(m);
	return;

error:
	return;
}

void *
marena_alloc(struct marena *m, ssize s)
{
	const usize alignment = alignof(max_align_t);
	ssize mem_size        = align_up_size_t(s);
	ptrdiff_t p           = (ptrdiff_t)m->p;
	ptrdiff_t aligned     = (p + (alignment - 1)) & ~(alignment - 1);
	ssize padding         = aligned - p;

	if(padding > m->rem - mem_size) {
		return NULL;
	}

	m->p = (void *)(aligned + mem_size);
	m->rem -= padding + mem_size;

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
	m->p       = (char *)p;
	ssize offs = ((char *)p - (char *)m->buf);

	m->rem = m->buf_size - offs;
}

void
marena_reset(struct marena *m)
{
	m->p   = (char *)m->buf;
	m->rem = m->buf_size;
}

void *
marena_alloc_rem(struct marena *m, ssize *s)
{
	// If size is > 0 s_out = the remainder
	if(s) *s = m->rem;

	void *mem = m->p;
	m->p += m->rem;
	m->rem = 0;
	return mem;
}

ssize
marena_size_rem(struct marena *m)
{
	return m->rem;
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
