#pragma once

#include "base/mathfunc.h"
#include "engine/gfx/gfx.h"
#include <stdlib.h>

#include "base/mem.h"
#include "base/types.h"

#include "base/utils.h"
#include "base/str.h"
#include "sys/sys.h"
#include "base/dbg.h"

// #define PROF_UNIQUE_NAMES
#if defined(PROF)

#define PROF_HISTORY_SIZE 128 // number of frames of history to keep
#define PROF_ANCHORS_SIZE 100 // number of unique zones allowed in the entire application
#define PROF_FRAMES_SIZE  64  // Number of call depth allowed

#if defined(PROF_UNIQUE_NAMES)
#define prof_stringize_2(x) #x
#define prof_stringize(x)   prof_stringize_2(x)
#define prof_unique_name(name) \
	name "_" prof_stringize(__LINE__)

#define prof_block(name) prof_block_start(prof_unique_name(name), __COUNTER__ + 1)
/* #define prof_block(name) prof_block_start(name, \
 	({ static int i = -1; if (i == -1) i = prof_next_block_idx(); i; })) */
#else
#define prof_block(name) prof_block_start(name, __COUNTER__ + 1)
#endif

#define prof_block_func() prof_block(__func__)
#define prof_block_end()  prof_block_end_internal()

#else

#define PROF_ANCHORS_SIZE 1
#define PROF_HISTORY_SIZE 1
#define PROF_FRAMES_SIZE  1
#define prof_block(...)
#define prof_block_func(...)
#define prof_block_end(...)
#endif

struct prof_anchor {
	u32 us_exclusive; // Does not include children
	u32 us_inclusive; // Does include children

	u32 hit_count;

	const char *label;
};

struct prof_frame {
	u16 anchor_idx;
	u16 parent_idx;

	u32 us_start;
	u32 prev_us_inclusive;

	u32 child_time;
	const char *label;
};

struct prof {
	u16 parent_idx;

	u32 us_start;
	u32 us_end;

	struct prof_anchor anchors[PROF_ANCHORS_SIZE];
	struct prof_frame frames[PROF_FRAMES_SIZE];
	// u32 history[PROF_ANCHORS_SIZE][PROF_HISTORY_SIZE]; // 256K

	ssize anchor_count;
	ssize frame_count;
};

static struct prof PROFILER;
static char INT_TO_STRING[100][4];
static char INT_TO_STRING_DECIMAL[100][4];
static char INT_TO_STRING_MID_DECIMAL[100][4];
static void
int_to_string_ini(void)
{
	int i;
	for(i = 0; i < 100; ++i) {
		sprintf(INT_TO_STRING[i], "%d", i);
		sprintf(INT_TO_STRING_DECIMAL[i], ".%02d", i);
		sprintf(INT_TO_STRING_MID_DECIMAL[i], "%d.%d", i / 10, i % 10);
	}
}

static inline void prof_f32_to_str8(u8 *buf, f32 value, i32 precision);

static void
prof_ini(void)
{
	struct prof *prof = &PROFILER;
	dbg_assert(ARRLEN(prof->anchors) < U16_MAX);
	dbg_assert(ARRLEN(prof->frames) < U16_MAX);
	mclr_struct(prof);
	prof->us_start = sys_time_us();
}

void
prof_close(void)
{
	struct prof *prof = &PROFILER;
	prof->us_end      = sys_time_us();
}

void
prof_block_start(const char *label, ssize idx)
{
	struct prof *prof = &PROFILER;
	dbg_assert(idx < (ssize)ARRLEN(prof->anchors));
	prof->anchor_count = MAX(idx + 1, prof->anchor_count);

	struct prof_anchor *item = prof->anchors + idx;
	dbg_assert(prof->frame_count < (ssize)ARRLEN(prof->frames));
	prof->frames[prof->frame_count++] = (struct prof_frame){
		.prev_us_inclusive = item->us_inclusive,
		.us_start          = sys_time_us(),
		.parent_idx        = prof->parent_idx,
		.anchor_idx        = idx,
		.label             = label,
	};

	prof->parent_idx = idx;
}

void
prof_block_end_internal(void)
{
	u32 now_us        = sys_time_us();
	struct prof *prof = &PROFILER;
	if(prof->frame_count == 0) { return; } // if we just initialized in the middle a profiler then ignore this block

	struct prof_frame *frame = &prof->frames[--prof->frame_count];

	u32 elapsed      = now_us - frame->us_start;
	prof->parent_idx = frame->parent_idx;

	struct prof_anchor *parent = prof->anchors + frame->parent_idx;
	struct prof_anchor *anchor = prof->anchors + frame->anchor_idx;

	parent->us_exclusive -= elapsed;
	anchor->us_exclusive += elapsed;
	anchor->us_inclusive = frame->prev_us_inclusive + elapsed;
	++anchor->hit_count;
	anchor->label = frame->label;
}

#define PROF_REPORT_NUM_VALUES 3
#define PROF_REPORT_NUM_TITLES 2
#define PROF_REPORT_NUM_HEADER (PROF_REPORT_NUM_VALUES + 1)

struct prof_report_entry {
	str8 label;
	union {
		struct {
			u32 us_exclusive;
			u32 us_inclusive;
			u32 hit_count;
		};
		u32 values[PROF_REPORT_NUM_VALUES];
	};
};

// Title is: 32.180 ms/frame (fps: 31.14) sort self - current time
// Header is zone self hier count
struct prof_report {
	u32 total_time;
	str8 titles[PROF_REPORT_NUM_TITLES];
	str8 headers[PROF_REPORT_NUM_HEADER];
	i32 hilight;

	ssize entry_count;
	struct prof_report_entry entries[PROF_ANCHORS_SIZE];
};

static int
prof_report_sort_inclusive_desc(const void *a, const void *b)
{
	const struct prof_report_entry *aa = a;
	const struct prof_report_entry *bb = b;
	dbg_assert(aa->hit_count > 0);
	dbg_assert(bb->hit_count > 0);
	f32 a_value = (f32)aa->us_exclusive / (f32)(aa->hit_count > 0 ? aa->hit_count : 1);
	f32 b_value = (f32)bb->us_exclusive / (f32)(aa->hit_count > 0 ? bb->hit_count : 1);

	if(b_value > a_value) return 1;
	if(b_value < a_value) return -1;
	return 0;
}

struct prof_report *
prof_report_create(struct alloc alloc)
{
	struct prof *prof       = &PROFILER;
	struct prof_report *res = alloc_struct_clr(alloc, res);

	u32 now         = prof->us_end ? prof->us_end : sys_time_us();
	res->total_time = now - prof->us_start;

	/* ------------------------------------------------------------
	   Copy anchors so we don't mutate profiler state
	------------------------------------------------------------ */
	struct prof_anchor temp[PROF_ANCHORS_SIZE];
	mcpy(temp, prof->anchors, sizeof(temp));

	/* ------------------------------------------------------------
	   Account for currently open frames
	------------------------------------------------------------ */
	for(ssize i = 0; i < prof->frame_count; ++i) {
		struct prof_frame *frame = &prof->frames[i];

		u32 elapsed = now - frame->us_start;

		struct prof_anchor *anchor = &temp[frame->anchor_idx];

		anchor->us_inclusive = frame->prev_us_inclusive + elapsed;
		anchor->us_exclusive += elapsed;
	}

	/* ------------------------------------------------------------
	   Collect valid anchors
	------------------------------------------------------------ */
	for(ssize i = 0; i < prof->anchor_count; ++i) {
		struct prof_anchor *a = &temp[i];

		if(!a->label) continue;
		if(a->hit_count == 0 && a->us_inclusive == 0) continue;

		struct prof_report_entry *e =
			&res->entries[res->entry_count++];

		e->label        = str8_cstr((char *)a->label);
		e->hit_count    = a->hit_count;
		e->us_inclusive = a->us_inclusive;
		e->us_exclusive = a->us_exclusive;
	}

	/* ------------------------------------------------------------
	   Sort
	------------------------------------------------------------ */
	qsort(res->entries,
		res->entry_count,
		sizeof(struct prof_report_entry),
		prof_report_sort_inclusive_desc);

	// TODO: Title should change depending if the report is sorted by self or hier, and if history is enabled should show how many frames
	// res->titles[0] = str8_fmt_push(
	// 	alloc,
	// 	"%3.3lf ms/frame (fps: (%e.2lf) %s",
	// 	(double)0.2f,
	// 	50,
	// 	"sort self - current frame");
	res->titles[0] = str8_fmt_push(alloc, "%s", "sort exclusive - current frame");

	// TODO: Show warning on wrong timer?

	res->headers[0] = str8_lit("zone");
	res->headers[1] = str8_lit("excl");
	res->headers[2] = str8_lit("incl");
	res->headers[3] = str8_lit("count");

	return res;
}

void
prof_drw(
	struct alloc alloc,
	struct gfx_ctx ctx,
	i32 sx,
	i32 sy,
	i32 full_width,
	i32 height,
	i32 line_spacing,
	i32 precision,
	void (*txt_drw)(i32 x, i32 y, str8 txt, i32 spr_mode),
	i32 (*txt_width)(str8 str))
{
	i32 field_width = txt_width(str8_lit("5555.55"));
	i32 name_width  = full_width - field_width * 3;

	if(!INT_TO_STRING[0][0])
		int_to_string_ini();
	struct prof_report *report = prof_report_create(alloc);

#if 0
	for(ssize i = 0; i < (ssize)ARRLEN(report->titles); ++i) {
		if(report->titles[i].size > 0) {
			i32 title_x0 = sx;
			i32 title_x1 = title_x0 + full_width;
			gfx_rec_fill(ctx, title_x0, sy, full_width, line_spacing + 2, PRIM_MODE_BLACK);

			txt_drw(sx + 2, sy + 1, report->titles[i], SPR_MODE_WHITE);

			sy += line_spacing;
			height -= abs_i32(line_spacing);
		}
	}
#endif

	i32 max_records  = height / abs_i32(line_spacing);
	i32 record_count = min_i32(report->entry_count, max_records);

#if 1
	gfx_rec_fill(ctx, sx, sy, full_width, line_spacing - 1, PRIM_MODE_WHITE);
	if(report->headers[0].size > 0) {
		txt_drw(sx, sy, report->headers[0], SPR_MODE_BLACK);
	}

	for(ssize j = 1; j < (ssize)ARRLEN(report->headers); ++j) {
		if(report->headers[j].size > 0) {
			txt_drw(
				sx + name_width + field_width * (j - 1) + field_width / 2 - txt_width(report->headers[j]) / 2,
				sy,
				report->headers[j],
				SPR_MODE_BLACK);
		}
	}
	sy += line_spacing;
#endif

	// Draw bg
	gfx_rec_fill(ctx, sx, sy, full_width, line_spacing * record_count, PRIM_MODE_BLACK);
	for(ssize i = 0; i < record_count; ++i) {
		u8 buf[256];
		u8 *b                       = buf;
		struct prof_report_entry *r = &report->entries[i];
		i32 x                       = sx;

		txt_drw(x + 1, sy, r->label, SPR_MODE_WHITE);
		for(ssize j = 0; j < (ssize)ARRLEN(r->values); ++j) {
			if(j == 2) {
				// hit count stays integer
				sys_sprintf((char *)buf, "%" PRIu32, r->values[j]);
			} else {
				i32 hit_count = r->hit_count;
				f32 avg_ms    = 0.0f;

				if(hit_count > 0) {
					f32 us = (f32)r->values[j];
					avg_ms = (us / (f32)hit_count) / 1000.0f;
				}

				prof_f32_to_str8(buf, avg_ms, precision);
			}

			txt_drw(sx + name_width + field_width * j, sy, str8_cstr((char *)buf), SPR_MODE_WHITE);
		}

		sy += line_spacing;
	}
}

str8
prof_str8(struct alloc alloc)
{
	struct prof *prof     = &PROFILER;
	struct str8_list list = {0};
	u32 us_end            = prof->us_end;
	if(!us_end) {
		us_end = sys_time_us();
	}

	u32 total_time = us_end - prof->us_start;

	// Header: 60 columns exactly
	// Name(14) Hits(7) Tot(9) Exc(9) Avg(9) %(6)
	str8_list_pushf(alloc, &list, "%-12s %7s %9s %9s %9s %6s", "Block", "Hits", "Tot(us)", "Exc(us)", "Avg(us)", "Pct");
	str8_list_pushf(alloc, &list, "%.*s", 60, "------------------------------------------------------------");

	f32 acc_perc = 0.0f;
	dbg_assert(prof->frame_count == 0);

	for(size_t i = 0; i < ARRLEN(prof->anchors); i++) {
		struct prof_anchor *item = prof->anchors + i;

		if(item->label == NULL || item->hit_count == 0) { continue; }
		// dbg_assert(item->frame_count == 0);

		f32 percent  = total_time ? (100.0f * (f32)item->us_exclusive / (f32)total_time) : 0.0f;
		f32 avg_excl = (f32)item->us_exclusive / (f32)item->hit_count;
		acc_perc += percent;

		str8_list_pushf(
			alloc,
			&list,
			"%-12.12s %7" PRIu32 " %9" PRIu32 " %9" PRIu32 " %9.2f %5.1f%%",
			item->label,
			item->hit_count,
			item->us_inclusive,
			item->us_exclusive,
			(double)avg_excl,
			(double)percent);
	}

	str8_list_pushf(alloc, &list, "%.*s", 60, "------------------------------------------------------------");
	str8_list_pushf(
		alloc,
		&list,
		"%-12s %7s %9" PRIu32 " %9s %9s %5.1f%%",
		"TOTAL TIME",
		"",
		total_time,
		"",
		"",
		(double)acc_perc);

	struct str_join params = {.sep = str8_lit("\n")};
	return str8_list_join(alloc, &list, &params);
}

static inline void
prof_f32_to_str8(u8 *buf, f32 value, i32 precision)
{
	i32 x, y;
	static char *formats[5] = {"%.0f", "%.1f", "%.2f", "%.3f", "%.4f"};
	char *b                 = (char *)buf;
	switch(precision) {
	case 2:
		if(value < 0 || value >= 100)
			break;
		x = value;
		y = (value - x) * 100;
		strcpy(b, INT_TO_STRING[x]);
		strcat(b, INT_TO_STRING_DECIMAL[y]);
		return;
	case 3:
		if(value < 0 || value >= 10)
			break;
		value *= 10;
		x = value;
		y = (value - x) * 100;
		strcpy(b, INT_TO_STRING_MID_DECIMAL[x]);
		strcat(b, INT_TO_STRING_DECIMAL[y] + 1);
		return;
	case 4:
		if(value < 0 || value >= 1)
			break;
		value *= 100;
		x    = value;
		y    = (value - x) * 100;
		b[0] = '0';
		strcpy(b + 1, INT_TO_STRING_DECIMAL[x]);
		strcat(b, INT_TO_STRING_DECIMAL[y] + 1);
		return;
	}
	sys_sprintf(b, formats[precision], (double)value);
}
