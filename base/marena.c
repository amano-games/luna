#include "base/dbg.h"
#include "marena.h"

void
marena_ini_internal(struct marena *m, void *buf, ssize buf_size, u32 flags, const char *name, const char *file, i32 line)
{
	dbg_check_mem(buf, "Marena");

	m->buf      = buf;
	m->buf_size = buf_size;

#if MARENA_DEBUG
	m->peak  = 0;
	m->flags = flags;
	m->name  = name;
	m->file  = file;
	m->line  = line;
#endif

	marena_reset(m);

error:
	return;
}

void
marena_set_name(struct marena *m, const char *name)
{
#if MARENA_DEBUG
	m->name = name;
#endif
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

#if MARENA_DEBUG
	m->peak = MAX(m->peak, m->buf_size - m->rem);
#endif

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

#if MARENA_DEBUG
	m->peak = m->buf_size;
#endif

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
