#include "base/dbg.h"
#include "mem-prof.h"
#include "lib/csv.h"
#include <stdlib.h>

static void mem_prof_report_sort(struct mem_prof_report *report, enum mem_prof_sort sort);

struct mem_prof_table MEM_PROF_TABLE = {0};

static struct mem_prof_info *
mem_prof_info_get(struct marena *m)
{
	for(ssize i = 0; i < MEM_PROF_TABLE.count; ++i) {
		if(MEM_PROF_TABLE.infos[i].arena == m) { return &MEM_PROF_TABLE.infos[i]; }
	}
	return NULL;
}

static i32
mem_prof_info_idx(struct marena *m)
{
	for(ssize i = 0; i < MEM_PROF_TABLE.count; ++i) {
		if(MEM_PROF_TABLE.infos[i].arena == m) { return (i32)i; }
	}
	return -1;
}

static b32
mem_prof_is_ancestor(struct marena *m, struct marena *of)
{
	for(struct mem_prof_info *info = mem_prof_info_get(of); info && info->parent; info = mem_prof_info_get(info->parent)) {
		if(info->parent == m) { return true; }
	}
	return false;
}

static struct marena *
mem_prof_parent_find(struct marena *m)
{
	struct marena *res = NULL;
	for(ssize i = 0; i < MEM_PROF_TABLE.count; ++i) {
		struct marena *other = MEM_PROF_TABLE.infos[i].arena;
		if(other == m) { continue; }
		if(m->buf < other->buf || m->buf + m->buf_size > other->buf + other->buf_size) { continue; }
		if(mem_prof_is_ancestor(m, other)) { continue; }
		if(res == NULL || other->buf_size <= res->buf_size) {
			res = other;
		}
	}
	return res;
}

void
mem_prof_track(struct marena *m, str8 name, const char *file, i32 line)
{
	if(!MEM_PROF) { return; }

	struct mem_prof_info *info = mem_prof_info_get(m);

	if(!info) {
		if(MEM_PROF_TABLE.count >= MEM_PROF_TABLE_CAP) {
			static b32 warned = false;
			if(!warned) {
				warned = true;
				log_warn("mem-prof", "arena table full, %.*s is not tracked", str8_spread(name));
			}
			return;
		}
		info        = &MEM_PROF_TABLE.infos[MEM_PROF_TABLE.count++];
		info->arena = m;
	}

	info->name   = name;
	info->file   = file;
	info->line   = line;
	info->parent = mem_prof_parent_find(m);
}

void
marena_ini_track_site(struct marena *m, void *buf, ssize size, str8 name, const char *file, i32 line)
{
	marena_ini(m, buf, size);
	mem_prof_track(m, name, file, line);
}

void
mem_prof_untrack(struct marena *m)
{
	for(ssize i = 0; i < MEM_PROF_TABLE.count; ++i) {
		if(MEM_PROF_TABLE.infos[i].arena != m) { continue; }
		MEM_PROF_TABLE.infos[i] = MEM_PROF_TABLE.infos[--MEM_PROF_TABLE.count];
		break;
	}
	for(ssize i = 0; i < MEM_PROF_TABLE.count; ++i) {
		if(MEM_PROF_TABLE.infos[i].parent == m) {
			MEM_PROF_TABLE.infos[i].parent = NULL;
		}
	}
}

void
mem_prof_untrack_in_memory(void *buf, ssize size)
{
	u8 *lo = (u8 *)buf;
	u8 *hi = lo + size;
	for(ssize i = MEM_PROF_TABLE.count - 1; i >= 0; --i) {
		u8 *p = (u8 *)MEM_PROF_TABLE.infos[i].arena->buf;
		if(p >= lo && p < hi) { mem_prof_untrack(MEM_PROF_TABLE.infos[i].arena); }
	}
}

void
mem_prof_name_set(struct marena *m, str8 name)
{
	struct mem_prof_info *info = mem_prof_info_get(m);
	if(info) { info->name = name; }
}

static void
mem_prof_report_entry_push(struct mem_prof_report *report, struct mem_prof_info *info, i32 depth)
{
	struct marena *m                = info->arena;
	struct mem_prof_info *parent    = info->parent ? mem_prof_info_get(info->parent) : NULL;
	struct mem_prof_report_entry *e = &report->entries[report->entry_count];
	e->order                        = report->entry_count++;
	e->arena                        = m;
	e->name                         = info->name;
	e->parent_name                  = parent ? parent->name : (str8){0};
	e->file                         = info->file;
	e->line                         = info->line;
	e->depth                        = depth;
	e->used                         = marena_size_used(m);
	e->peak                         = m->peak;
	e->size                         = m->buf_size;
}

struct mem_prof_report *
mem_prof_report_create(struct alloc alloc, enum mem_prof_sort sort, u32 cols)
{
	dbg_assert(sort < MEM_PROF_SORT_NUM_COUNT);
	struct mem_prof_report *res = alloc_struct_clr(alloc, res);

	if(!res) { return NULL; }

	res->sort = sort;
	res->cols = (cols | MEM_PROF_COL_BIT(MEM_PROF_COL_NAME)) & MEM_PROF_COL_ALL;

	ssize count                   = MEM_PROF_TABLE.count;
	b8 pushed[MEM_PROF_TABLE_CAP] = {0};
	i32 parents[MEM_PROF_TABLE_CAP];

	for(ssize i = 0; i < count; ++i) {
		struct marena *parent = MEM_PROF_TABLE.infos[i].parent;
		parents[i]            = parent ? mem_prof_info_idx(parent) : -1;
	}

	i32 stack[MEM_PROF_TABLE_CAP];
	i32 depths[MEM_PROF_TABLE_CAP];

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
			mem_prof_report_entry_push(res, &MEM_PROF_TABLE.infos[i], depth);

			for(ssize child = count - 1; child >= 0; --child) {

				if(parents[child] != i || pushed[child] || top >= MEM_PROF_TABLE_CAP) { continue; }

				stack[top]    = (i32)child;
				depths[top++] = depth + 1;
			}
		}
	}

	for(ssize i = 0; i < count; ++i) {
		if(!pushed[i]) { mem_prof_report_entry_push(res, &MEM_PROF_TABLE.infos[i], 0); }
	}

	for(u32 i = 0; i < res->entry_count; ++i) {
		struct mem_prof_report_entry *e = &res->entries[i];
		if(e->depth != 0) { continue; }
		res->total_used += e->used;
		res->total_peak += e->peak;
		res->total_size += e->size;
	}

	mem_prof_report_sort(res, sort);
	return res;
}

static int
mem_prof_report_perc_cmp(ssize a, ssize as, ssize b, ssize bs)
{
	i64 l = as > 0 ? (i64)a * (bs > 0 ? bs : 1) : 0;
	i64 r = bs > 0 ? (i64)b * (as > 0 ? as : 1) : 0;
	return (l > r) - (l < r);
}

static int
mem_prof_report_order_cmp(const struct mem_prof_report_entry *a, const struct mem_prof_report_entry *b)
{
	return (a->order > b->order) - (a->order < b->order);
}

static int
mem_prof_report_sort_used_perc(const void *pa, const void *pb)
{
	const struct mem_prof_report_entry *a = pa;
	const struct mem_prof_report_entry *b = pb;
	int res                               = mem_prof_report_perc_cmp(b->used, b->size, a->used, a->size);
	return res != 0 ? res : mem_prof_report_order_cmp(a, b);
}

static int
mem_prof_report_sort_free_perc(const void *pa, const void *pb)
{
	const struct mem_prof_report_entry *a = pa;
	const struct mem_prof_report_entry *b = pb;
	int res                               = mem_prof_report_perc_cmp(b->size - b->used, b->size, a->size - a->used, a->size);
	return res != 0 ? res : mem_prof_report_order_cmp(a, b);
}

static int
mem_prof_report_sort_size(const void *pa, const void *pb)
{
	const struct mem_prof_report_entry *a = pa;
	const struct mem_prof_report_entry *b = pb;
	int res                               = (b->size > a->size) - (b->size < a->size);
	return res != 0 ? res : mem_prof_report_order_cmp(a, b);
}

static void
mem_prof_report_sort(struct mem_prof_report *report, enum mem_prof_sort sort)
{
	int (*cmp)(const void *, const void *) = NULL;
	switch(sort) {
	case MEM_PROF_SORT_USED_PERC: cmp = mem_prof_report_sort_used_perc; break;
	case MEM_PROF_SORT_FREE_PERC: cmp = mem_prof_report_sort_free_perc; break;
	case MEM_PROF_SORT_SIZE: cmp = mem_prof_report_sort_size; break;
	default: break;
	}
	if(cmp) { qsort(report->entries, report->entry_count, sizeof(report->entries[0]), cmp); }
}

str8
mem_prof_csv(struct alloc alloc, enum mem_prof_sort sort)
{
	str8 alloc_name = {0};

	if(alloc.allocf == marena_alloc_func) {
		struct mem_prof_info *info = mem_prof_info_get((struct marena *)alloc.ctx);
		if(info) { alloc_name = info->name; }
	}

	struct mem_prof_report *report = mem_prof_report_create(alloc, sort, MEM_PROF_COL_ALL);

	if(!report) { return (str8){0}; }

	struct str8_list list = {0};
	str8_list_pushf(
		alloc,
		&list,
		"# arenas=%u cap=%d mem_prof=%d sort=%d alloc=%.*s used=%" PRIi64 " peak=%" PRIi64 " size=%" PRIi64 "\n",
		report->entry_count,
		(i32)MEM_PROF_TABLE_CAP,
		(i32)MEM_PROF,
		(i32)report->sort,
		str8_spread(alloc_name),
		(i64)report->total_used,
		(i64)report->total_peak,
		(i64)report->total_size);

	str8_list_pushf(alloc, &list, "name,parent,depth,used,peak,size,file,line\n");

	for(u32 i = 0; i < report->entry_count; ++i) {
		struct mem_prof_report_entry *e = &report->entries[i];
		str8 name                       = csv_quote_push(alloc, e->name);
		str8 parent                     = csv_quote_push(alloc, e->parent_name);
		str8 file                       = csv_quote_push(alloc, e->file ? str8_cstr((char *)e->file) : (str8){0});
		str8_list_pushf(
			alloc,
			&list,
			"%.*s,%.*s,%d,%" PRIi64 ",%" PRIi64 ",%" PRIi64 ",%.*s,%d\n",
			str8_spread(name),
			str8_spread(parent),
			e->depth,
			(i64)e->used,
			(i64)e->peak,
			(i64)e->size,
			str8_spread(file),
			e->line);
	}

	return str8_list_join(alloc, &list, NULL);
}

b32
mem_prof_csv_save(struct alloc alloc, enum mem_prof_sort sort, str8 app_name, str8 app_org)
{
	if(!MEM_PROF) { return false; }
	str8 csv  = mem_prof_csv(alloc, sort);
	str8 path = csv_save(alloc, csv, str8_lit("mem"), app_name, app_org);
	if(path.size == 0) { return false; }
	log_info("mem-prof", "mem csv saved: %.*s", str8_spread(path));
	return true;
}
