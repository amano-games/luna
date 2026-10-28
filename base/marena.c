#include "base/dbg.h"
#include "marena.h"

static void marena_table_register(struct marena *m);

struct marena_table MARENA_TABLE = {0};

void
marena_ini_internal(struct marena *m, void *buf, ssize buf_size, u32 flags, const char *name, const char *file, i32 line)
{
	dbg_check_mem(buf, "Marena");

	m->buf      = buf;
	m->buf_size = buf_size;

#if MARENA_DEBUG
	m->peak   = 0;
	m->flags  = flags;
	m->name   = name;
	m->file   = file;
	m->line   = line;
	m->parent = NULL;
	if(!(flags & MARENA_FLAG_LOCAL)) {
		marena_table_register(m);
	}
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

// For an arena that dies before the program ends
void
marena_unregister(struct marena *m)
{
#if MARENA_DEBUG
	for(ssize i = 0; i < MARENA_TABLE.count; ++i) {
		if(MARENA_TABLE.arenas[i] != m) { continue; }
		MARENA_TABLE.arenas[i] = MARENA_TABLE.arenas[--MARENA_TABLE.count];
		break;
	}
	for(ssize i = 0; i < MARENA_TABLE.count; ++i) {
		if(MARENA_TABLE.arenas[i]->parent == m) {
			MARENA_TABLE.arenas[i]->parent = NULL;
		}
	}
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

static b32
marena_is_ancestor(struct marena *m, struct marena *of)
{
	for(struct marena *p = of->parent; p; p = p->parent) {
		if(p == m) { return true; }
	}
	return false;
}

static struct marena *
marena_table_parent_find(struct marena *m)
{
	struct marena *res = NULL;
	for(ssize i = 0; i < MARENA_TABLE.count; ++i) {
		struct marena *other = MARENA_TABLE.arenas[i];
		if(other == m) { continue; }
		if(m->buf < other->buf || m->buf + m->buf_size > other->buf + other->buf_size) { continue; }
		if(marena_is_ancestor(m, other)) { continue; }
		if(res == NULL || other->buf_size <= res->buf_size) {
			res = other;
		}
	}
	return res;
}

static void
marena_table_register(struct marena *m)
{
	b32 found = false;
	for(ssize i = 0; i < MARENA_TABLE.count; ++i) {
		if(MARENA_TABLE.arenas[i] == m) {
			found = true;
			break;
		}
	}

	if(!found) {
		if(MARENA_TABLE.count >= MARENA_TABLE_CAP) {
			static b32 warned = false;
			if(!warned) {
				warned = true;
				log_warn("marena", "arena table full, %s is not tracked", m->name);
			}
			return;
		}
		MARENA_TABLE.arenas[MARENA_TABLE.count++] = m;
	}

	m->parent = marena_table_parent_find(m);
}
