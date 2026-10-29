#include "base/dbg.h"
#include "marena.h"
#include "lib/csv.h"
#include "base/mem-prof.h"
#include <stdlib.h>

static void marena_table_register(struct marena *m);

struct marena_table MARENA_TABLE = {0};
struct mem_prof MEM_PROF         = {.cols = MEM_PROF_COL_ALL};

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

void
marena_unregister_in_memory(void *buf, ssize size)
{
	u8 *lo = (u8 *)buf;
	u8 *hi = lo + size;
	for(ssize i = MARENA_TABLE.count - 1; i >= 0; --i) {
		u8 *p = (u8 *)MARENA_TABLE.arenas[i]->buf;
		if(p >= lo && p < hi) { marena_unregister(MARENA_TABLE.arenas[i]); }
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
	struct marena_report_entry *e = &report->entries[report->entry_count];
	e->order                      = report->entry_count++;
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

	return res;
}

static int
marena_report_perc_cmp(ssize a, ssize as, ssize b, ssize bs)
{
	i64 l = as > 0 ? (i64)a * (bs > 0 ? bs : 1) : 0;
	i64 r = bs > 0 ? (i64)b * (as > 0 ? as : 1) : 0;
	return (l > r) - (l < r);
}

static int
marena_report_order_cmp(const struct marena_report_entry *a, const struct marena_report_entry *b)
{
	return (a->order > b->order) - (a->order < b->order);
}

static int
marena_report_sort_used_perc(const void *pa, const void *pb)
{
	const struct marena_report_entry *a = pa;
	const struct marena_report_entry *b = pb;
	int res                             = marena_report_perc_cmp(b->used, b->size, a->used, a->size);
	return res != 0 ? res : marena_report_order_cmp(a, b);
}

static int
marena_report_sort_free_perc(const void *pa, const void *pb)
{
	const struct marena_report_entry *a = pa;
	const struct marena_report_entry *b = pb;
	int res                             = marena_report_perc_cmp(b->size - b->used, b->size, a->size - a->used, a->size);
	return res != 0 ? res : marena_report_order_cmp(a, b);
}

static int
marena_report_sort_size(const void *pa, const void *pb)
{
	const struct marena_report_entry *a = pa;
	const struct marena_report_entry *b = pb;
	int res                             = (b->size > a->size) - (b->size < a->size);
	return res != 0 ? res : marena_report_order_cmp(a, b);
}

void
marena_report_sort(struct marena_report *report, enum marena_sort sort)
{
	int (*cmp)(const void *, const void *) = NULL;
	switch(sort) {
	case MARENA_SORT_USED_PERC: cmp = marena_report_sort_used_perc; break;
	case MARENA_SORT_FREE_PERC: cmp = marena_report_sort_free_perc; break;
	case MARENA_SORT_SIZE: cmp = marena_report_sort_size; break;
	default: break;
	}
	if(cmp) { qsort(report->entries, report->entry_count, sizeof(report->entries[0]), cmp); }
}

str8
marena_csv(struct alloc alloc)
{
	str8 alloc_name = {0};

	if(alloc.allocf == marena_alloc_func || marena_table_idx((struct marena *)alloc.ctx) >= 0) {
		alloc_name = ((struct marena *)alloc.ctx)->name;
	}

	struct marena_report *report = marena_report_create(alloc);

	if(!report) { return (str8){0}; }

	struct str8_list list = {0};
	str8_list_pushf(
		alloc,
		&list,
		"# arenas=%u cap=%d marena_track=%d alloc=%.*s used=%" PRIi64 " peak=%" PRIi64 " size=%" PRIi64 "\n",
		report->entry_count,
		(i32)MARENA_TABLE_CAP,
		(i32)MARENA_TRACK,
		str8_spread(alloc_name),
		(i64)report->total_used,
		(i64)report->total_peak,
		(i64)report->total_size);

	str8_list_pushf(alloc, &list, "name,parent,depth,used,peak,size,used_pct,peak_pct,file,line\n");

	for(u32 i = 0; i < report->entry_count; ++i) {
		struct marena_report_entry *e = &report->entries[i];
		str8 name                     = csv_quote_push(alloc, e->name);
		str8 parent                   = csv_quote_push(alloc, e->parent_name);
		str8 file                     = csv_quote_push(alloc, e->file ? str8_cstr((char *)e->file) : (str8){0});
		str8 used_pct                 = csv_perc_push(alloc, e->used, e->size);
		str8 peak_pct                 = csv_perc_push(alloc, e->peak, e->size);
		str8_list_pushf(
			alloc,
			&list,
			"%.*s,%.*s,%d,%" PRIi64 ",%" PRIi64 ",%" PRIi64 ",%.*s,%.*s,%.*s,%d\n",
			str8_spread(name),
			str8_spread(parent),
			e->depth,
			(i64)e->used,
			(i64)e->peak,
			(i64)e->size,
			str8_spread(used_pct),
			str8_spread(peak_pct),
			str8_spread(file),
			e->line);
	}

	return str8_list_join(alloc, &list, NULL);
}

b32
marena_csv_save(struct alloc alloc, str8 app_name, str8 app_org)
{
	if(!MARENA_TRACK) { return false; }
	str8 csv  = marena_csv(alloc);
	str8 path = csv_save(alloc, csv, str8_lit("mem"), app_name, app_org);
	if(path.size == 0) { return false; }
	log_info("marena", "mem csv saved: %.*s", str8_spread(path));
	return true;
}
