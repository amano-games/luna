#include "mem.h"
#include "dbg.h"
#include "mem-arena.h"

void
marena_init(struct marena *m, void *buf, usize bufsize)
{
	dbg_check_mem(buf, "Marena");
	mspan sp    = {buf, bufsize};
	sp          = mspan_align(sp);
	m->buf_og   = buf;
	m->buf      = sp.p;
	m->buf_size = sp.size;

	marena_reset(m);
	return;

error:
	return;
}

void *
marena_alloc(struct marena *m, usize s)
{
	usize size = align_up_size_t(s);

	if(m->rem < size) { return NULL; }

	void *mem = m->p;
	m->p += size;
	m->rem -= size;

	return mem;
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
	usize offs = ((char *)p - (char *)m->buf);

	m->rem = m->buf_size - offs;
}

void
marena_reset(struct marena *m)
{
	m->p   = (char *)m->buf;
	m->rem = m->buf_size;
}

void *
marena_alloc_rem(struct marena *m, usize *s)
{
	// If size is > 0 s_out = the remainder
	if(s) *s = m->rem;

	void *mem = m->p;
	m->p += m->rem;
	m->rem = 0;
	return mem;
}

usize
marena_size_rem(struct marena *m)
{
	return m->rem;
}
