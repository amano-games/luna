#include "base/dbg.h"
#include "marena.h"

static void marena_table_register(struct marena *m);

struct marena_table MARENA_TABLE = {0};

#define MARENA_POISONS(m) (BUILD_ASAN && !((m)->flags & MARENA_FLAG_LOCAL))

void
marena_ini_internal(struct marena *m, void *buf, ssize buf_size, u32 flags, str8 name, const char *file, i32 line)
{
	dbg_check_mem(buf, "Marena");

	m->buf      = buf;
	m->buf_size = buf_size;

	m->peak   = 0;
	m->flags  = flags;
	m->name   = name;
	m->file   = file;
	m->line   = line;
	m->parent = NULL;
	if(MARENA_TRACK && !(flags & MARENA_FLAG_LOCAL)) {
		marena_table_register(m);
	}

	m->p = m->buf;
	if(MARENA_POISONS(m)) { MARENA_POISON(m->buf, m->buf_size); }
	marena_reset(m);

error:
	return;
}

void
marena_unregister(struct marena *m)
{
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

	if(MARENA_TRACK) { m->peak = MAX(m->peak, m->buf_size - m->rem); }

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

	if(MARENA_TRACK) { m->peak = m->buf_size; }

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
				log_warn("marena", "arena table full, %.*s is not tracked", str8_spread(m->name));
			}
			return;
		}
		MARENA_TABLE.arenas[MARENA_TABLE.count++] = m;
	}

	m->parent = marena_table_parent_find(m);
}

static i32
marena_table_idx(struct marena *m)
{
	for(ssize i = 0; i < MARENA_TABLE.count; ++i) {
		if(MARENA_TABLE.arenas[i] == m) { return (i32)i; }
	}
	return -1;
}

static void
marena_report_entry_push(struct marena_report *report, struct marena *m, i32 depth)
{
	struct marena_report_entry *e = &report->entries[report->entry_count++];
	e->arena                      = m;
	e->name                       = m->name;
	e->parent_name                = m->parent ? m->parent->name : (str8){0};
	e->file                       = m->file;
	e->line                       = m->line;
	e->depth                      = depth;
	e->used                       = marena_size_used(m);
	e->peak                       = m->peak;
	e->size                       = m->buf_size;
}

struct marena_report *
marena_report_create(struct alloc alloc)
{
	struct marena_report *res = alloc_struct_clr(alloc, res);

	if(!res) { return NULL; }

	ssize count                 = MARENA_TABLE.count;
	b8 pushed[MARENA_TABLE_CAP] = {0};
	i32 parents[MARENA_TABLE_CAP];

	for(ssize i = 0; i < count; ++i) {
		struct marena *parent = MARENA_TABLE.arenas[i]->parent;
		parents[i]            = parent ? marena_table_idx(parent) : -1;
	}

	// Tree order: each root in table order, followed by its children depth first.
	// The stack holds table indices; children are pushed in reverse so they pop in table order
	i32 stack[MARENA_TABLE_CAP];
	i32 depths[MARENA_TABLE_CAP];

	for(ssize root = 0; root < count; ++root) {

		if(parents[root] != -1) { continue; }

		ssize top     = 0;
		stack[top]    = (i32)root;
		depths[top++] = 0;

		while(top > 0) {
			--top;
			i32 i     = stack[top];
			i32 depth = depths[top];

			if(pushed[i]) { continue; }

			pushed[i] = true;
			marena_report_entry_push(res, MARENA_TABLE.arenas[i], depth);

			for(ssize child = count - 1; child >= 0; --child) {

				if(parents[child] != i || pushed[child] || top >= MARENA_TABLE_CAP) { continue; }

				stack[top]    = (i32)child;
				depths[top++] = depth + 1;
			}
		}
	}

	for(ssize i = 0; i < count; ++i) {
		if(!pushed[i]) { marena_report_entry_push(res, MARENA_TABLE.arenas[i], 0); }
	}

	for(u32 i = 0; i < res->entry_count; ++i) {
		struct marena_report_entry *e = &res->entries[i];
		if(e->depth != 0) { continue; }
		res->total_used += e->used;
		res->total_peak += e->peak;
		res->total_size += e->size;
	}

	{
		char used_b[STR_BYTES_CAP], peak_b[STR_BYTES_CAP], size_b[STR_BYTES_CAP];
		str8 used = str8_from_bytes_buf(used_b, sizeof(used_b), (u64)res->total_used);
		str8 peak = str8_from_bytes_buf(peak_b, sizeof(peak_b), (u64)res->total_peak);
		str8 size = str8_from_bytes_buf(size_b, sizeof(size_b), (u64)res->total_size);

		res->titles[0] = str8_fmt_push(alloc, "arenas %d/%d", (i32)count, (i32)MARENA_TABLE_CAP);
		res->titles[1] = str8_fmt_push(alloc, "used %.*s peak %.*s size %.*s", str8_spread(used), str8_spread(peak), str8_spread(size));

		res->headers[0] = str8_lit("arena");
		res->headers[1] = str8_lit("used");
		res->headers[2] = str8_lit("peak");
		res->headers[3] = str8_lit("size");
	}

	return res;
}
