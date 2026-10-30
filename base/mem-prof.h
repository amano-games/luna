#pragma once

#include "base/marena.h"
#include "base/mathfunc.h"
#include "base/mem.h"
#include "base/str.h"
#include "base/types.h"
#include "engine/gfx/gfx.h"
#include "sys/sys-printf.h"

#if !defined(MEM_PROF)
#define MEM_PROF BUILD_DEBUG
#endif

#define MEM_PROF_TABLE_CAP (MEM_PROF ? 64 : 1)

struct mem_prof_info {
	struct marena *arena;
	str8 name;
	const char *file;
	i32 line;
	struct marena *parent;
};

struct mem_prof_table {
	struct mem_prof_info infos[MEM_PROF_TABLE_CAP];
	ssize count;
};

extern struct mem_prof_table MEM_PROF_TABLE;

#define MEM_PROF_NAME(s) (MEM_PROF ? str8_lit(s) : (str8){0})
#define MEM_PROF_FILE    (MEM_PROF ? __FILE__ : NULL)
#define MEM_PROF_SITE(s) MEM_PROF_NAME(s), MEM_PROF_FILE, __LINE__

#if MEM_PROF
#define marena_ini_track(m, buf, size, name) marena_ini_track_site((m), (buf), (size), MEM_PROF_SITE(name))
#else
#define marena_ini_track(m, buf, size, name) marena_ini((m), (buf), (size))
#endif

void marena_ini_track_site(struct marena *m, void *buf, ssize size, str8 name, const char *file, i32 line);
void mem_prof_track(struct marena *m, str8 name, const char *file, i32 line);
void mem_prof_untrack(struct marena *m);
void mem_prof_untrack_in_memory(void *buf, ssize size);
void mem_prof_name_set(struct marena *m, str8 name);

#define MEM_PROF_REPORT_NUM_VALUES 3

enum mem_prof_sort {
	MEM_PROF_SORT_NONE,

	MEM_PROF_SORT_DEPTH,
	MEM_PROF_SORT_USED_PERC,
	MEM_PROF_SORT_FREE_PERC,
	MEM_PROF_SORT_SIZE,

	MEM_PROF_SORT_NUM_COUNT,
};

enum mem_prof_col {
	MEM_PROF_COL_NONE,

	MEM_PROF_COL_NAME,
	MEM_PROF_COL_USED,
	MEM_PROF_COL_PEAK,
	MEM_PROF_COL_SIZE,
	MEM_PROF_COL_PERC,
	MEM_PROF_COL_BAR,

	MEM_PROF_COL_NUM_COUNT,
};

#define MEM_PROF_COL_BIT(col) (1u << (col))
#define MEM_PROF_COL_ALL      (MEM_PROF_COL_BIT(MEM_PROF_COL_NUM_COUNT) - 1)

struct mem_prof_report_entry {
	struct marena *arena;
	str8 name;
	str8 parent_name;
	const char *file;
	i32 line;
	i32 depth;
	u32 order;
	union {
		struct {
			ssize used;
			ssize peak;
			ssize size;
		};
		ssize values[MEM_PROF_REPORT_NUM_VALUES];
	};
};

struct mem_prof_report {
	enum mem_prof_sort sort;
	u32 cols;
	ssize total_used;
	ssize total_peak;
	ssize total_size;
	u32 entry_count;
	struct mem_prof_report_entry entries[MEM_PROF_TABLE_CAP];
};

struct mem_prof_report *mem_prof_report_create(struct alloc alloc, enum mem_prof_sort sort, u32 cols);
str8 mem_prof_csv(struct alloc alloc, enum mem_prof_sort sort);
b32 mem_prof_csv_save(struct alloc alloc, enum mem_prof_sort sort, str8 app_name, str8 app_org);

static inline ssize
mem_prof_bytes_div(ssize ref)
{
	i64 div = 1;
	if(ref >= MKILOBYTE(1)) { div = MKILOBYTE(1); }
	if(ref >= MMEGABYTE(1)) { div = MMEGABYTE(1); }
	if(ref >= MGIGABYTE(1)) { div = MGIGABYTE(1); }
	if(div > 1 && div < MGIGABYTE(1) && ((i64)ref * 10 + div / 2) / div >= 10240) { div *= 1024; }
	return (ssize)div;
}

static inline char
mem_prof_bytes_unit(ssize div)
{
	if(div >= MGIGABYTE(1)) { return 'G'; }
	if(div >= MMEGABYTE(1)) { return 'M'; }
	if(div >= MKILOBYTE(1)) { return 'K'; }
	return 'B';
}

static inline int
mem_prof_bytes_num(char *buf, int cap, ssize v, ssize div)
{
	if(div == 1) { return sys_snprintf(buf, cap, "%lld", (long long)v); }
	i64 tenths = ((i64)v * 10 + div / 2) / div;
	return sys_snprintf(buf, cap, "%lld.%lld", (long long)(tenths / 10), (long long)(tenths % 10));
}

// 512B, 1.5K, 2.4M
static inline str8
mem_prof_bytes_short_buf(char *buf, usize cap, ssize bytes)
{
	ssize div = mem_prof_bytes_div(bytes);
	int n     = mem_prof_bytes_num(buf, (int)cap, bytes, div);
	n += sys_snprintf(buf + n, (int)cap - n, "%c", mem_prof_bytes_unit(div));
	return str8_cstr_capped(buf, buf + n);
}

// used/total in the unit of total: 13.8/18.2M
static inline str8
mem_prof_bytes_ratio_buf(char *buf, usize cap, ssize used, ssize total)
{
	ssize div = mem_prof_bytes_div(total);
	int n     = mem_prof_bytes_num(buf, (int)cap, used, div);
	n += sys_snprintf(buf + n, (int)cap - n, "/");
	n += mem_prof_bytes_num(buf + n, (int)cap - n, total, div);
	n += sys_snprintf(buf + n, (int)cap - n, "%c", mem_prof_bytes_unit(div));
	return str8_cstr_capped(buf, buf + n);
}

#define MEM_PROF_DRW_PADDING       2
#define MEM_PROF_DRW_COL_GAP       1
#define MEM_PROF_DRW_NAME_CHARS    10
#define MEM_PROF_DRW_PERC_CHARS    4 // 100%
#define MEM_PROF_DRW_BYTES_CHARS   7 // 1023.9K
#define MEM_PROF_DRW_MIN_BAR_CHARS 4

#if MEM_PROF

/*
 *  mem_prof_drw -- arena table overlay, sorted and filtered by the report
 *
 *    <sx, sy>      --  top left of the overlay
 *    full_width    --  overlay width, 0 fits every column;
 *    height        --  overlay height
 *    line_spacing  --  pitch of each row
 */
static inline void
mem_prof_drw(
	struct mem_prof_report *report,
	struct gfx_ctx ctx,
	i32 sx,
	i32 sy,
	i32 full_width,
	i32 height,
	i32 line_spacing,
	void (*txt_drw)(i32 x, i32 y, str8 txt, i32 spr_mode),
	i32 (*txt_width)(str8 str))
{
	if(!report) { return; }

	struct {
		enum mem_prof_col col; // Byte columns index entry.values from MEM_PROF_COL_USED
		str8 title;
		i32 chars;
		b32 right;
	} cols[] = {
		{MEM_PROF_COL_NAME, str8_lit("name"), MEM_PROF_DRW_NAME_CHARS, false},
		{MEM_PROF_COL_BAR, str8_lit("bar"), MEM_PROF_DRW_MIN_BAR_CHARS, false},
		{MEM_PROF_COL_PERC, str8_lit("perc"), MEM_PROF_DRW_PERC_CHARS, true},
		{MEM_PROF_COL_USED, str8_lit("used"), MEM_PROF_DRW_BYTES_CHARS, true},
		{MEM_PROF_COL_SIZE, str8_lit("size"), MEM_PROF_DRW_BYTES_CHARS, true},
		{MEM_PROF_COL_PEAK, str8_lit("peak"), MEM_PROF_DRW_BYTES_CHARS, true},
	};

	// The sorted columns' titles are drawn inverted; depth (tree) order highlights the name,
	// and the used percent also highlights the bar since it shows the same ratio
	static const u32 sort_cols[MEM_PROF_SORT_NUM_COUNT] = {
		[MEM_PROF_SORT_DEPTH]     = MEM_PROF_COL_BIT(MEM_PROF_COL_NAME),
		[MEM_PROF_SORT_USED_PERC] = MEM_PROF_COL_BIT(MEM_PROF_COL_PERC) | MEM_PROF_COL_BIT(MEM_PROF_COL_BAR),
		[MEM_PROF_SORT_FREE_PERC] = MEM_PROF_COL_BIT(MEM_PROF_COL_PERC),
		[MEM_PROF_SORT_SIZE]      = MEM_PROF_COL_BIT(MEM_PROF_COL_SIZE),
	};

	u32 visible = report->cols;
	u32 sorted  = sort_cols[report->sort];
	i32 cw      = txt_width(str8_lit("0"));
	i32 gap     = MEM_PROF_DRW_COL_GAP * cw;
	i32 row_h   = line_spacing;
	i32 min_w   = MEM_PROF_DRW_PADDING * 2 - gap;

	for(ssize j = 0; j < (ssize)ARRLEN(cols); ++j) {
		min_w += cols[j].chars * cw + gap;
	}

	i32 w     = full_width > 0 ? full_width : min_w;
	i32 x0    = sx + MEM_PROF_DRW_PADDING;
	i32 x1    = sx + w - MEM_PROF_DRW_PADDING;
	i32 y     = sy + MEM_PROF_DRW_PADDING;
	i32 y_end = sy + height - MEM_PROF_DRW_PADDING;
	i32 fit   = max_i32(0, (x1 - x0) / cw);

	// Column layout, shared by the title and entry rows. A width of 0 means not drawn:
	// hidden, or past the right edge.
	i32 col_x[ARRLEN(cols)] = {0};
	i32 col_w[ARRLEN(cols)] = {0};

	{
		i32 avail  = x1 - x0;
		i32 used_w = 0; // Placed columns other than the bar
		i32 placed = 0;
		ssize bar  = -1;

		for(ssize j = 0; j < (ssize)ARRLEN(cols); ++j) {
			if(!(visible & MEM_PROF_COL_BIT(cols[j].col))) { continue; }
			if(cols[j].col == MEM_PROF_COL_BAR) {
				bar = j;
				continue;
			}
			i32 left   = avail - used_w - placed * gap;
			i32 cell_w = cols[j].chars * cw;

			if(cols[j].col == MEM_PROF_COL_NAME) { cell_w = min_i32(cell_w, left); }
			if(cell_w <= 0 || cell_w > left) { break; }

			col_w[j] = cell_w;
			used_w += cell_w;
			++placed;
		}

		if(bar >= 0) {
			i32 bar_w  = avail - used_w - placed * gap;
			col_w[bar] = bar_w >= cols[bar].chars * cw ? bar_w : 0;
		}

		i32 x = x0;
		for(ssize j = 0; j < (ssize)ARRLEN(cols); ++j) {
			if(col_w[j] == 0) { continue; }
			col_x[j] = x;
			x += col_w[j] + gap;
		}
	}

	gfx_rec_fill(ctx, sx, sy, w, height, PRIM_MODE_BLACK);

	if(y + row_h > y_end) { return; }

	{
		ssize used = report->total_used;
		ssize size = report->total_size;
		i32 perc   = size > 0 ? (i32)((used * 100) / size) : 0;
		char ratio_b[32], peak_b[16], buf[64];
		int n = sys_snprintf(
			buf,
			sizeof(buf),
			"Totals: %3d%% %.*s peak %.*s",
			(int)perc,
			str8_spread(mem_prof_bytes_ratio_buf(ratio_b, sizeof(ratio_b), used, size)),
			str8_spread(mem_prof_bytes_short_buf(peak_b, sizeof(peak_b), report->total_peak)));
		txt_drw(x0, y, str8_prefix(str8_cstr_capped(buf, buf + n), (usize)fit), SPR_MODE_WHITE);
		y += line_spacing;
	}

	if(y + row_h > y_end) { return; }

	for(ssize j = 0; j < (ssize)ARRLEN(cols); ++j) {
		if(col_w[j] == 0) { continue; }
		str8 title  = str8_prefix(cols[j].title, (usize)(col_w[j] / cw));
		i32 title_x = col_x[j] + (cols[j].right ? (cols[j].chars - (i32)title.size) * cw : 0);
		if(sorted & MEM_PROF_COL_BIT(cols[j].col)) {
			gfx_rec_fill(ctx, col_x[j], y, col_w[j], row_h, PRIM_MODE_WHITE);
			txt_drw(title_x + 1, y, title, SPR_MODE_BLACK);
		} else {
			txt_drw(title_x, y, title, SPR_MODE_WHITE);
		}
	}

	y += line_spacing;

	for(u32 i = 0; i < report->entry_count && y + row_h <= y_end; ++i) {
		struct mem_prof_report_entry *entry = &report->entries[i];

		for(ssize j = 0; j < (ssize)ARRLEN(cols); ++j) {
			if(col_w[j] == 0) { continue; }
			i32 x      = col_x[j];
			i32 cell_w = col_w[j];

			switch(cols[j].col) {
			default: {
				char bytes_b[16], buf[16];
				ssize bytes = entry->values[cols[j].col - MEM_PROF_COL_USED];
				int n       = sys_snprintf(buf, sizeof(buf), "%7.*s", str8_spread(mem_prof_bytes_short_buf(bytes_b, sizeof(bytes_b), bytes)));
				txt_drw(x, y, str8_cstr_capped(buf, buf + n), SPR_MODE_WHITE);
			} break;

			case MEM_PROF_COL_NAME: {
				txt_drw(x, y, str8_prefix(entry->name, (usize)((x1 - x) / cw)), SPR_MODE_WHITE);
			} break;

			case MEM_PROF_COL_PERC: {
				if(entry->size > 0) {
					char buf[8];
					int n = sys_snprintf(buf, sizeof(buf), "%3d%%", (int)((entry->used * 100) / entry->size));
					txt_drw(x, y, str8_cstr_capped(buf, buf + n), SPR_MODE_WHITE);
				}
			} break;

			case MEM_PROF_COL_BAR: {
				gfx_rec(ctx, x, y + 1, cell_w, row_h - 2, PRIM_MODE_WHITE);
				if(entry->size > 0) {
					i32 used_w = (i32)((entry->used * cell_w) / entry->size);
					i32 peak_x = (i32)((entry->peak * (cell_w - 1)) / entry->size);
					gfx_rec_fill(ctx, x, y + 1, used_w, row_h - 2, PRIM_MODE_WHITE);
					gfx_rec_fill(ctx, x + peak_x, y, 1, row_h, PRIM_MODE_INV);
				}
			} break;
			}
		}
		y += line_spacing;
	}
}

#else

static inline void
mem_prof_drw(
	struct mem_prof_report *report,
	struct gfx_ctx ctx,
	i32 sx,
	i32 sy,
	i32 full_width,
	i32 height,
	i32 line_spacing,
	void (*txt_drw)(i32 x, i32 y, str8 txt, i32 spr_mode),
	i32 (*txt_width)(str8 str))
{
}

#endif
