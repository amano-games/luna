#pragma once

#include "base/mathfunc.h"
#include "engine/gfx/gfx.h"
#include <stdlib.h>

#include "base/mem.h"
#include "base/types.h"

#include "base/utils.h"
#include "base/str.h"
#include "sys/sys-io.h"
#include "sys/sys.h"
#include "sys/sys-intrin.h"
#include "base/dbg.h"

// #define PROF_UNIQUE_NAMES

enum prof_anchor_sys {
	PROF_ANCHOR_SYS_NONE,

	PROF_ANCHOR_SYS_UPD,
	PROF_ANCHOR_SYS_DRW,
	PROF_ANCHOR_SYS_PROF,

	PROF_ANCHOR_SYS_NUM_COUNT,
};

// Maps to prof_report_entry.values[] so the comparator can index the column.
enum prof_sort {
	PROF_SORT_EXCLUSIVE,
	PROF_SORT_INCLUSIVE,

	PROF_SORT_NUM_COUNT,
};

// Maps to prof.smooth_slot / PROF_TRACKER_HISTORY_SLOTS.
enum prof_smooth {
	PROF_SMOOTH_INSTANT,
	PROF_SMOOTH_FAST,
	PROF_SMOOTH_SLOW,

	PROF_SMOOTH_NUM_COUNT,
};

#if PROF

#define PROF_ANCHORS_SIZE        96    // number of unique zones allowed in the entire application
#define PROF_FRAMES_SIZE         64    // Number of call depth allowed
#define PROF_INT_ZERO_THRESHHOLD 0.25f // threshhold for a moving average of an integer to be at zero

#if defined(PROF_UNIQUE_NAMES)
#define prof_unique_name(name) name "_" LUNA_STRINGIFY(__LINE__)

#define prof_block(name) \
	do { \
		static ssize LUNA_CONCAT(prof_idx_, __LINE__) = -1; \
		if(LUNA_CONCAT(prof_idx_, __LINE__) < 0) { \
			LUNA_CONCAT(prof_idx_, __LINE__) = prof_next_block_idx(); \
		} \
		prof_block_start(prof_unique_name(name), LUNA_CONCAT(prof_idx_, __LINE__)); \
	} while(0)
#else
// Per-call-site static index (C99). __LINE__ only disambiguates the static's
// name; the stable anchor id is assigned once via prof_next_block_idx().
#define prof_block(name) \
	do { \
		static ssize LUNA_CONCAT(prof_idx_, __LINE__) = -1; \
		if(LUNA_CONCAT(prof_idx_, __LINE__) < 0) { \
			LUNA_CONCAT(prof_idx_, __LINE__) = prof_next_block_idx(); \
		} \
		prof_block_start((name), LUNA_CONCAT(prof_idx_, __LINE__)); \
	} while(0)
#endif

#define prof_block_func() prof_block(__func__)
#define prof_block_end()  prof_block_end_internal()

#else

#define PROF_ANCHORS_SIZE 1
#define PROF_FRAMES_SIZE  1
#define prof_block(...)
#define prof_block_func(...)
#define prof_block_end(...)
#endif

#define PROF_TRACKER_HISTORY_SLOTS   3
#define PROF_THROWAWAY_UPDATES_COUNT 3
#define PROF_FRAME_TIME_INITIAL_US   1000u // 1ms, iprof FRAME_TIME_INITIAL 0.001s

struct prof_hist_scalar {
	f32 values[PROF_TRACKER_HISTORY_SLOTS];
	f32 variances[PROF_TRACKER_HISTORY_SLOTS];
};

struct prof_anchor {
	u32 us_exclusive; // Does not include children
	u32 us_inclusive; // Does include children

	u32 hit_count;
};

struct prof_hist_slot {
	u32 dt_us;
	struct prof_anchor anchors[PROF_ANCHORS_SIZE];
};

struct prof_anchor_totals {
	u64 sum_exc;
	u64 sum_inc;
	u64 sum_hits;
};

struct prof_anchor_hist {
	u32 us_inclusive_min;
	u32 us_inclusive_max;

	struct prof_hist_scalar us_exclusive;
	struct prof_hist_scalar us_inclusive;
	struct prof_hist_scalar hit_count;
};

struct prof_frame {
	u16 anchor_idx;
	u16 parent_idx;

	u32 us_start;
	u32 prev_us_inclusive;

	const char *label;
};

struct prof {
	u16 parent_idx;
	u16 next_anchor;

	u8 sort;        // For report
	u8 smooth_slot; // For report

	u32 update_idx; // 2^31 at 100fps = 280 days
	u32 last_upd_us;
	u32 frame_dt_us;
	u32 frame_sum_us;

	const char *anchor_labels[PROF_ANCHORS_SIZE];
	struct prof_anchor anchors[PROF_ANCHORS_SIZE];
	struct prof_anchor_hist anchors_hist[PROF_ANCHORS_SIZE];
	struct prof_frame frames[PROF_FRAMES_SIZE];

	struct prof_hist_scalar frame_time;

	// 1s present-rate. Each prof_upd is one present: dt_us is added to the window
	// and present_counter is incremented. On crossing 1s, present_per_s latches the
	// count and present_window_us keeps the leftover (subtract 1s, do not reset).
	// Throwaway startup frames zero all three. present_per_s is the report title's
	// N/s; 0 means the first window has not closed yet (title shows 0).
	u16 present_counter;   // presents in the current 1s window
	u16 present_per_s;     // presents in the last completed 1s window
	u32 present_window_us; // accumulated dt_us toward 1s

	u16 anchor_count;
	u16 frame_count;

	u32 timer_call_us;

#if PROF_HISTORY
	u16 history_idx; // next write; also oldest slot (iProf history_index)
#endif
#if PROF_HISTORY == PROF_HISTORY_FRAME
	u32 capture_n;
	struct prof_anchor_totals totals[PROF_ANCHORS_SIZE];
#endif
};

#define PROF_REPORT_NUM_VALUES 5
#define PROF_REPORT_NUM_TITLES 3
#define PROF_REPORT_NUM_HEADER (PROF_REPORT_NUM_VALUES + 1)

struct prof_report_entry {
	const char *label;
	u16 zone;
	union {
		struct {
			f32 ms_exclusive;
			f32 ms_inclusive;
			f32 hit_count;
			f32 ms_inclusive_min;
			f32 ms_inclusive_max;
		};
		f32 values[PROF_REPORT_NUM_VALUES];
	};
};

// Title 0: frame <ms>  <n>/s  gap <ms>
// Header is zone excl incl count
struct prof_report {
	str8 titles[PROF_REPORT_NUM_TITLES];
	str8 headers[PROF_REPORT_NUM_HEADER];
	u32 entry_count;
	struct prof_report_entry entries[PROF_ANCHORS_SIZE];
};

// Defined once in sys.c — must be shared across luna/game TUs.
extern struct prof PROFILER;
#if PROF_HISTORY == PROF_HISTORY_FRAME
extern struct prof_hist_slot PROF_FRAME_HIST[PROF_HISTORY_SIZE];
#elif PROF_HISTORY == PROF_HISTORY_ZONE
extern u32 PROF_ZONE_EXCL[PROF_HISTORY_SIZE][PROF_ANCHORS_SIZE];
#endif

#if PROF

static f32 PROF_TIMES_TO_REACH_90_PERCENT[PROF_TRACKER_HISTORY_SLOTS];
static f32 PROF_PRECOMPUTED_FACTORS[PROF_TRACKER_HISTORY_SLOTS];

static char INT_TO_STRING[100][4];
static char INT_TO_STRING_DECIMAL[100][4];
static char INT_TO_STRING_MID_DECIMAL[100][4];
static str8 INT_TO_STR8[100];
static str8 INT_TO_STR8_DECIMAL[100];
static str8 INT_TO_STR8_MID_DECIMAL[100];

#if PROF_HISTORY == PROF_HISTORY_ZONE
static inline void prof_zone_history_push(struct prof *prof);
#endif

static inline void
int_to_string_ini(void)
{
	int i;
	for(i = 0; i < 100; ++i) {
		// INT
		int len             = sys_sprintf(INT_TO_STRING[i], "%d", i);
		INT_TO_STR8[i].str  = (u8 *)INT_TO_STRING[i];
		INT_TO_STR8[i].size = len;

		// .XX
		len                         = sys_sprintf(INT_TO_STRING_DECIMAL[i], ".%02d", i);
		INT_TO_STR8_DECIMAL[i].str  = (u8 *)INT_TO_STRING_DECIMAL[i];
		INT_TO_STR8_DECIMAL[i].size = len;

		// X.X
		len                             = sys_sprintf(INT_TO_STRING_MID_DECIMAL[i], "%d.%d", i / 10, i % 10);
		INT_TO_STR8_MID_DECIMAL[i].str  = (u8 *)INT_TO_STRING_MID_DECIMAL[i];
		INT_TO_STR8_MID_DECIMAL[i].size = len;
	}
}

static inline str8 prof_f32_to_str8(u8 *buf, f32 value, i32 precision);

static inline void prof_history_scalar_upd(struct prof_hist_scalar *h, f32 sample, f32 *factors);
static inline void prof_history_scalar_eternity(struct prof_hist_scalar *h, f32 new_value);
static inline void prof_rec_fill(struct gfx_ctx ctx, i32 x, i32 y, i32 w, i32 h, enum prim_mode mode);

static inline ssize
prof_next_block_idx(void)
{
	struct prof *prof = &PROFILER;
	dbg_assert(prof->next_anchor >= (u16)PROF_ANCHOR_SYS_NUM_COUNT);
	dbg_assert(prof->next_anchor < (ssize)ARRLEN(prof->anchors));
	return prof->next_anchor++;
}

#define PROF_CALIBRATE_LOOP_COUNT 1000
static inline u32
prof_calibrate_timer_us(void)
{
	u32 t0 = sys_time_us();
	for(i32 i = 0; i < PROF_CALIBRATE_LOOP_COUNT; ++i) {
		(void)sys_time_us();
	}
	u32 t1 = sys_time_us();
	return (t1 - t0) / (u32)PROF_CALIBRATE_LOOP_COUNT;
}

static inline void
prof_ini(void)
{
	struct prof *prof = &PROFILER;
	dbg_assert(ARRLEN(prof->anchors) < U16_MAX);
	dbg_assert(ARRLEN(prof->frames) < U16_MAX);

	u16 next_anchor = prof->next_anchor;
	if(next_anchor < (u16)PROF_ANCHOR_SYS_NUM_COUNT) {
		next_anchor = (u16)PROF_ANCHOR_SYS_NUM_COUNT;
	}

	mclr_struct(prof);
#if PROF_HISTORY == PROF_HISTORY_FRAME
	mclr_array(PROF_FRAME_HIST);
#elif PROF_HISTORY == PROF_HISTORY_ZONE
	mclr_array(PROF_ZONE_EXCL);
#endif

	prof->timer_call_us = prof_calibrate_timer_us();

	{
		PROF_TIMES_TO_REACH_90_PERCENT[0] = 0.1f;
		PROF_TIMES_TO_REACH_90_PERCENT[1] = 0.8f;
		PROF_TIMES_TO_REACH_90_PERCENT[2] = 2.5f;
	}
	{
		for(ssize i = 0; i < (ssize)ARRLEN(prof->frame_time.values); ++i) {
			prof->frame_time.values[i] = (f32)PROF_FRAME_TIME_INITIAL_US;
		}
	}
	prof->next_anchor = next_anchor;
	prof->smooth_slot = PROF_SMOOTH_FAST;
	prof->sort        = PROF_SORT_EXCLUSIVE;
}

static inline void
prof_sort_set(enum prof_sort sort)
{
	dbg_assert(sort < PROF_SORT_NUM_COUNT);
	PROFILER.sort = (u16)sort;
}

static inline enum prof_sort
prof_sort_get(void)
{
	return (enum prof_sort)PROFILER.sort;
}

static inline void
prof_smooth_set(enum prof_smooth smooth)
{
	dbg_assert(smooth < PROF_SMOOTH_NUM_COUNT);
	PROFILER.smooth_slot = (u16)smooth;
}

static inline void
prof_block_start(const char *label, ssize idx)
{
	struct prof *prof = &PROFILER;
	dbg_assert(idx >= 0 && idx < (ssize)ARRLEN(prof->anchors));
	dbg_assert(prof->frame_count < (ssize)ARRLEN(prof->frames));

	prof->anchor_count                = MAX(idx + 1, prof->anchor_count);
	struct prof_anchor *item          = prof->anchors + idx;
	prof->frames[prof->frame_count++] = (struct prof_frame){
		.prev_us_inclusive = item->us_inclusive,
		.us_start          = sys_time_us(),
		.parent_idx        = prof->parent_idx,
		.anchor_idx        = idx,
		.label             = label,
	};

	prof->parent_idx = idx;
}

static inline void
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

	// dbg_assert(elapsed < 100000l);
	parent->us_exclusive -= elapsed;
	anchor->us_exclusive += elapsed;
	anchor->us_inclusive = frame->prev_us_inclusive + elapsed;
	++anchor->hit_count;
	prof->anchor_labels[frame->anchor_idx] = frame->label;
}

static inline int
prof_report_sort_desc(const void *a, const void *b)
{
	const struct prof_report_entry *aa = a;
	const struct prof_report_entry *bb = b;
	u16 col                            = PROFILER.sort;
	f32 a_value                        = aa->values[col];
	f32 b_value                        = bb->values[col];

	if(b_value > a_value) return 1;
	if(b_value < a_value) return -1;
	return 0;
}

static inline struct prof_report *
prof_report_create(struct alloc alloc)
{
	struct prof_report *res = alloc_struct(alloc, res);
	struct prof *prof       = &PROFILER;

	if(!INT_TO_STRING[0][0])
		int_to_string_ini();

	u32 presents = prof->present_per_s;
	f32 frame_ms = 0;
	if(presents != 0) {
		frame_ms = 1000.0f / (f32)presents;
	}
	f32 gap_ms = prof->frame_time.values[prof->smooth_slot] * 1e-3f;

	res->entry_count = 0;
	for(ssize i = 1; i < prof->anchor_count; ++i) {
		struct prof_anchor *a      = prof->anchors + i;
		struct prof_anchor_hist *h = prof->anchors_hist + i;
		const char *label          = prof->anchor_labels[i];
		f32 count                  = h->hit_count.values[prof->smooth_slot];

		if(!label) { continue; }
		if(count < PROF_INT_ZERO_THRESHHOLD) { continue; }

		struct prof_report_entry *e = &res->entries[res->entry_count++];
		e->label                    = label;
		e->zone                     = (u16)i;
		e->hit_count                = h->hit_count.values[prof->smooth_slot];
		e->ms_inclusive             = h->us_inclusive.values[prof->smooth_slot] * 1e-3f;
		e->ms_exclusive             = h->us_exclusive.values[prof->smooth_slot] * 1e-3f;
		e->ms_inclusive_min         = h->us_inclusive_min * 1e-3f;
		e->ms_inclusive_max         = h->us_inclusive_max * 1e-3f;
	}

	qsort(res->entries,
		res->entry_count,
		sizeof(struct prof_report_entry),
		prof_report_sort_desc);

	{
		u8 frame_buf[32];
		u8 gap_buf[32];
		str8 frame_str = prof_f32_to_str8(frame_buf, frame_ms, 0);
		str8 gap_str   = prof_f32_to_str8(gap_buf, gap_ms, 1);

		res->titles[0] = str8_fmt_push(alloc, "frame %sms  %u/s  gap %sms", frame_str.str, presents, gap_str.str);
		res->titles[1] = str8_lit("");
		res->titles[2] = str8_lit("");

		res->headers[0] = str8_lit("zone");
		res->headers[1] = str8_lit("excl");
		res->headers[2] = str8_lit("incl");
		res->headers[3] = str8_lit("count");
	}

	return res;
}

static inline void
prof_upd(b32 record_data)
{
	struct prof *prof = &PROFILER;

	// Check if all zones are closed
	dbg_assert(prof->frame_count == 0);
	dbg_assert(prof->parent_idx == 0);

	prof_block_start("_prof", PROF_ANCHOR_SYS_PROF);
	u32 now_us = prof->frames[prof->frame_count - 1].us_start;
	u32 dt_us  = 0;

	if(prof->update_idx > 0) {
		dt_us = now_us - prof->last_upd_us;
	}
	if(dt_us == 0) {
		dt_us = PROF_FRAME_TIME_INITIAL_US;
	}

	prof->last_upd_us = now_us;
	prof->frame_dt_us = dt_us;
	f32 dt_seconds    = (f32)dt_us * 1e-6f;

	{
		// Update PROF_PRECOMPUTED_FACTORS using the current dt
		for(ssize i = 1; i < (ssize)ARRLEN(PROF_TIMES_TO_REACH_90_PERCENT); ++i) {
			dbg_assert(PROF_TIMES_TO_REACH_90_PERCENT[i] != 0);
			PROF_PRECOMPUTED_FACTORS[i] = pow_f32(0.1f, dt_seconds / PROF_TIMES_TO_REACH_90_PERCENT[i]);
		}
		PROF_PRECOMPUTED_FACTORS[0] = 0.0f; // instantaneous
	}

#if 0
	// calculate the total time between frames to use for normalization and percentage calculations later on
	{
		if(prof->update_idx == 0) {
			prof->frame_sum_us = 0;
			for(ssize i = 1; i < prof->anchor_count; ++i) {
				prof->frame_sum_us += prof->anchors[i].us_exclusive;
			}
		} else {
			prof->frame_sum_us = dt_us;
		}
		if(prof->frame_sum_us == 0) {
			prof->frame_sum_us = 1;
		}
	}
#endif

	if(record_data) {
#if PROF_HISTORY == PROF_HISTORY_FRAME
		struct prof_hist_slot *hist_slot = 0;
		if(prof->update_idx >= PROF_THROWAWAY_UPDATES_COUNT) {
			hist_slot = &PROF_FRAME_HIST[prof->history_idx];
		}
#endif
		{
			// Prof_traverse(update_history);
			for(ssize i = 1; i < prof->anchor_count; ++i) {
				struct prof_anchor *a      = &prof->anchors[i];
				struct prof_anchor_hist *h = &prof->anchors_hist[i];
				const char *label          = prof->anchor_labels[i];

				if(prof->update_idx < PROF_THROWAWAY_UPDATES_COUNT) {
					prof_history_scalar_eternity(&h->us_exclusive, (f32)a->us_exclusive);
					prof_history_scalar_eternity(&h->us_inclusive, (f32)a->us_inclusive);
					prof_history_scalar_eternity(&h->hit_count, (f32)a->hit_count);
					h->us_inclusive_min = UINT32_MAX;
					h->us_inclusive_max = 0;
				} else {
					prof_history_scalar_upd(&h->us_exclusive, (f32)a->us_exclusive, PROF_PRECOMPUTED_FACTORS);
					prof_history_scalar_upd(&h->us_inclusive, (f32)a->us_inclusive, PROF_PRECOMPUTED_FACTORS);
					prof_history_scalar_upd(&h->hit_count, (f32)a->hit_count, PROF_PRECOMPUTED_FACTORS);
					h->us_inclusive_min = MIN(h->us_inclusive_min, a->us_inclusive);
					h->us_inclusive_max = MAX(h->us_inclusive_max, a->us_inclusive);
				}

#if PROF_HISTORY == PROF_HISTORY_FRAME
				if(hist_slot) {
					hist_slot->anchors[i]        = *a;
					struct prof_anchor_totals *t = &prof->totals[i];
					t->sum_exc += a->us_exclusive;
					t->sum_inc += a->us_inclusive;
					t->sum_hits += a->hit_count;
				}
#endif
			}
		}

		{
			if(prof->update_idx < PROF_THROWAWAY_UPDATES_COUNT) {
				// Avoid smoothing when the profiler is starting up
				prof_history_scalar_eternity(&prof->frame_time, (f32)dt_us);
				prof->present_counter   = 0;
				prof->present_per_s     = 0;
				prof->present_window_us = 0;
			} else {
				prof_history_scalar_upd(&prof->frame_time, (f32)dt_us, PROF_PRECOMPUTED_FACTORS);
				prof->present_window_us += dt_us;
				prof->present_counter++;
				if(1000000u <= prof->present_window_us) {
					prof->present_window_us -= 1000000u;
					prof->present_per_s   = prof->present_counter;
					prof->present_counter = 0;
				}
			}
		}

#if PROF_HISTORY == PROF_HISTORY_FRAME
		if(hist_slot) {
			hist_slot->dt_us  = dt_us;
			prof->history_idx = (u16)((prof->history_idx + 1) % PROF_HISTORY_SIZE);
			++prof->capture_n;
		}
#elif PROF_HISTORY == PROF_HISTORY_ZONE
		if(prof->update_idx >= PROF_THROWAWAY_UPDATES_COUNT) {
			prof_zone_history_push(prof);
		}
#endif

		++prof->update_idx;
	}

	if(prof->anchor_count > 0) {
		mclr(prof->anchors + 1, (prof->anchor_count - 1) * sizeof(prof->anchors[0]));
	}

	// mclr wiped last gather's inclusive; this end is the only hit this present.
	dbg_assert(prof->frame_count == 1);
	prof->frames[prof->frame_count - 1].prev_us_inclusive = 0;
	prof_block_end();
}

#if PROF_HISTORY == PROF_HISTORY_ZONE
static inline void
prof_zone_history_push(struct prof *prof)
{
	u16 history_idx = prof->history_idx;
	u32 *col        = PROF_ZONE_EXCL[history_idx];
	for(ssize i = 1; i < prof->anchor_count; ++i) {
		col[i] = prof->anchors[i].us_exclusive;
	}
	prof->history_idx = (u16)((history_idx + 1) % ARRLEN(PROF_ZONE_EXCL));
}
#endif

#if PROF_HISTORY
static inline u16
prof_history_capacity(void)
{
	return PROF_HISTORY_SIZE;
}

static inline u16
prof_history_slot(u16 logical_i)
{
	dbg_assert(logical_i < PROF_HISTORY_SIZE);
	return (u16)((PROFILER.history_idx + logical_i) % PROF_HISTORY_SIZE);
}

static inline u32
prof_zone_excl_at(u16 zone, u16 logical_i)
{
	dbg_assert(zone < PROF_ANCHORS_SIZE);
#if PROF_HISTORY == PROF_HISTORY_FRAME
	return PROF_FRAME_HIST[prof_history_slot(logical_i)].anchors[zone].us_exclusive;
#else
	return PROF_ZONE_EXCL[prof_history_slot(logical_i)][zone];
#endif
}

#if PROF_HISTORY == PROF_HISTORY_FRAME
static inline const struct prof_hist_slot *
prof_history_logical_at(u16 logical_i)
{
	return &PROF_FRAME_HIST[prof_history_slot(logical_i)];
}

static inline u32
prof_capture_n(void)
{
	return PROFILER.capture_n;
}
#endif
#endif

static inline i32
prof_report_title_count(struct prof_report *report)
{
	i32 n = 0;
	for(ssize i = 0; i < (ssize)ARRLEN(report->titles); ++i) {
		if(report->titles[i].size > 0) {
			++n;
		}
	}
	return n;
}

static inline i32
prof_report_visible_count(struct prof_report *report, i32 height, i32 line_spacing)
{
	i32 ls          = abs_i32(line_spacing);
	i32 max_records = (height - prof_report_title_count(report) * ls) / ls;
	return min_i32((i32)report->entry_count, max_records);
}

static inline void
prof_drw(
	struct prof_report *report,
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
	prof_block("prof_drw");
	i32 pad          = 1;
	i32 field_width  = txt_width(str8_lit("5555.55"));
	i32 max_columns  = 3;
	i32 name_width   = full_width - (field_width * max_columns);
	i32 title_count  = prof_report_title_count(report);
	i32 record_count = prof_report_visible_count(report, height, line_spacing);
	i32 bg_h         = (title_count + 1) * line_spacing;
	precision        = clamp_i32(precision, 1, 4);

	if(record_count > 0) {
		bg_h += (line_spacing * record_count) + (pad * (record_count - 1));
	}
	prof_rec_fill(ctx, sx, sy, full_width, bg_h, PRIM_MODE_BLACK);

	for(ssize i = 0; i < (ssize)ARRLEN(report->titles); ++i) {
		if(report->titles[i].size > 0) {
			txt_drw(sx + pad, sy + pad, report->titles[i], SPR_MODE_WHITE);
			sy += line_spacing;
		}
	}

	if(report->headers[0].size > 0) {
		txt_drw(sx + pad, sy + pad, report->headers[0], SPR_MODE_WHITE);
	}

	i32 column_w = name_width + pad;
	for(ssize j = 1; j < max_columns + 1; ++j) {
		if(report->headers[j].size > 0) {
			i32 col_x    = sx + column_w + field_width * (j - 1);
			b32 selected = (j == (ssize)prof_sort_get() + 1);
			if(selected) {
				prof_rec_fill(ctx, sx + name_width + field_width * (j - 1), sy, field_width, line_spacing, PRIM_MODE_WHITE);
			}
			txt_drw(col_x, sy + pad, report->headers[j], selected ? SPR_MODE_BLACK : SPR_MODE_WHITE);
		}
	}

	if(record_count > 0) {
		sy += line_spacing;
		for(ssize i = 0; i < record_count; ++i) {
			u8 buf[64];
			str8 str;
			struct prof_report_entry *r = &report->entries[i];
			i32 x                       = sx;

			if(!r->label) { continue; }

			txt_drw(x + 1, sy, str8_cstr((char *)r->label), SPR_MODE_WHITE);
			for(ssize j = 0; j < max_columns; ++j) {
				str = prof_f32_to_str8(buf, r->values[j], j == 2 ? 2 : precision);
				txt_drw(sx + pad + name_width + field_width * j, sy + pad, str, SPR_MODE_WHITE);
			}

			sy += line_spacing + pad;
		}
	}
	prof_block_end();
}

#if PROF_HISTORY
static inline void
prof_graph_row(
	struct gfx_ctx ctx,
	i32 sx,
	i32 sy,
	i32 x_scale,
	i32 y_scale,
	i32 row_h,
	u32 *samples,
	u16 n)
{
	y_scale = max_i32(1, y_scale);

	for(u16 i = 0; i < n; ++i) {
		i32 x  = sx + (i32)i * x_scale;
		i32 x2 = x + x_scale - 1;

		if(x2 > ctx.clip_x2) { break; }

		i32 h = min_i32(row_h, (i32)((u64)samples[i] * (u64)y_scale / 1000u));

		if(h < 1) { continue; }

		prof_rec_fill(ctx, x, sy + row_h - h, x_scale, h, PRIM_MODE_WHITE);
	}
}

/*
 *  prof_graph_drw -- exclusive-time history next to the report
 *
 *    <sx, sy>      --  origin of the graph--location of (0,0)
 *    x_scale       --  screenspace size of each history sample; e.g.
 *                         2 pixels
 *    y_scale       --  screenspace size of one millisecond of time;
 *                         for an app with max of 20ms in any one zone,
 *                         8 would produce a 160-pixel tall display,
 *                         assuming screenspace is in pixels
 *    height        --  same budget as prof_drw (visible row count)
 *    line_spacing  --  how much to move sy by after each zone row
 */
static inline void
prof_graph_drw(
	struct prof_report *report,
	struct gfx_ctx ctx,
	i32 sx,
	i32 sy,
	i32 x_scale,
	i32 y_scale,
	i32 height,
	i32 line_spacing)
{
	prof_block("prof_graph");

	if(report == 0) { goto cleanup; }

	i32 pad          = 1;
	u16 n            = prof_history_capacity();
	i32 row_h        = abs_i32(line_spacing);
	i32 title_count  = prof_report_title_count(report);
	i32 record_count = prof_report_visible_count(report, height, line_spacing);
	i32 x_step       = max_i32(x_scale, 1);
	i32 max_w        = ctx.clip_x2 - sx + 1;
	i32 bg_h         = (title_count + 1) * line_spacing;

	if(record_count > 0) {
		bg_h += (line_spacing * record_count) + (pad * (record_count - 1));
	}

	if(n == 0 || row_h < 1 || bg_h < 1 || max_w < 1) { goto cleanup; }

	i32 width = min_i32(max_w, (i32)n * x_step);

	prof_rec_fill(ctx, sx, sy, width, bg_h, PRIM_MODE_BLACK);

	sy += (title_count + 1) * line_spacing;

	for(ssize row = 0; row < record_count; ++row) {
		u16 zone = report->entries[row].zone;
		u32 samples[PROF_HISTORY_SIZE];
		for(u16 i = 0; i < n; ++i) {
			samples[i] = prof_zone_excl_at(zone, i);
		}
		prof_graph_row(ctx, sx, sy, x_step, y_scale, row_h, samples, n);
		sy += line_spacing + pad;
	}

cleanup:;
	prof_block_end();
}
#endif

static inline void
prof_history_scalar_eternity(struct prof_hist_scalar *h, f32 new_value)
{
	f32 new_variance = new_value * new_value;
	for(ssize i = 0; i < (ssize)ARRLEN(h->variances); ++i) {
		h->values[i]    = new_value;
		h->variances[i] = new_variance;
	}
}

static inline void
prof_history_scalar_upd(struct prof_hist_scalar *h, f32 sample, f32 *factors)
{
	h->values[0]    = sample;
	h->variances[0] = 0.0f;

	for(ssize i = 1; i < (ssize)ARRLEN(h->variances); ++i) {
		f32 f        = factors[i];
		f32 old      = h->values[i];
		f32 new_val  = old * f + sample * (1.0f - f);
		h->values[i] = new_val;

		f32 old_var     = h->variances[i];
		f32 new_var     = old_var * f + (sample * sample) * (1.0f - f);
		h->variances[i] = new_var;
	}
}

static inline str8
prof_f32_to_str8(u8 *buf, f32 value, i32 precision)
{
	i32 x, y;
	str8 res                = {.str = buf, .size = 0};
	static char *formats[5] = {"%.0f", "%.1f", "%.2f", "%.3f", "%.4f"};
	switch(precision) {
	case 2: {
		if(value < 0 || value >= 100) break;

		x = value;
		y = (value - x) * 100;
		dbg_assert(x >= 0 && x < 100 && y >= 0 && y < 100);

		str8_cpy(&INT_TO_STR8[x], &res);
		str8_cat_in_place(&res, &INT_TO_STR8_DECIMAL[y]);
		return res;
	}
	case 3: {
		if(value < 0 || value >= 10) break;
		value *= 10.0f;

		x = value;
		y = (value - x) * 100;
		dbg_assert(x >= 0 && x < 100 && y >= 0 && y < 100);

		str8 b = INT_TO_STR8_DECIMAL[y];
		b.str += 1;
		b.size -= 1;
		str8_cpy(&INT_TO_STR8_MID_DECIMAL[x], &res);
		str8_cat_in_place(&res, &b);
		return res;
	}
	case 4: {
		if(value < 0 || value >= 1) break;

		value *= 100;
		x = value;
		y = (value - x) * 100;
		dbg_assert(x >= 0 && x < 100 && y >= 0 && y < 100);

		res.str[0] = '0';
		res.size   = 1;
		res.str[1] = 0;
		str8_cat_in_place(&res, &INT_TO_STR8_DECIMAL[x]);
		str8 b = INT_TO_STR8_DECIMAL[y];
		b.str += 1;
		b.size -= 1;
		str8_cat_in_place(&res, &b);
		return res;
	}
	}
	sys_sprintf((char *)buf, formats[precision], (double)value);
	res.size = cstr8_len(buf);
	return res;
}

static inline void
prof_apply_prim_mode_x(u32 *restrict dp, u32 sm, enum prim_mode mode)
{
	switch(mode) {
	case PRIM_MODE_WHITE: *dp |= sm; break;
	case PRIM_MODE_BLACK: *dp &= ~sm; break;
	default: dbg_sentinel("prof");
	}
error:;
}

static inline void
prof_rec_fill(struct gfx_ctx ctx, i32 x, i32 y, i32 w, i32 h, enum prim_mode mode)
{
	dbg_assert(w > 0 && h > 0);

	i32 x1 = x;
	i32 y1 = y;
	i32 x2 = x + w - 1;
	i32 y2 = y + h - 1;

	dbg_assert(x1 >= ctx.clip_x1);
	dbg_assert(y1 >= ctx.clip_y1);
	dbg_assert(x2 <= ctx.clip_x2);
	dbg_assert(y2 <= ctx.clip_y2);

	struct tex dtex = ctx.dst;
	dbg_assert(dtex.fmt == TEX_FMT_OPAQUE);

	u32 *base  = dtex.px;
	i32 stride = dtex.wword; // words per row

	i32 start_w = x1 >> 5;
	i32 end_w   = x2 >> 5;

	i32 start_bit = x1 & 31;
	i32 end_bit   = x2 & 31;

	// left mask
	u32 left_mask = bswap_u32(0xFFFFFFFFu >> start_bit);

	// right mask
	u32 right_mask = bswap_u32(0xFFFFFFFFu << (31 - end_bit));

	for(i32 row = y1; row <= y2; row++) {
		u32 *dp = base + row * stride + start_w;

		if(start_w == end_w) {
			// Rectangle fits inside one word
			u32 mask = left_mask & right_mask;
			prof_apply_prim_mode_x(dp, mask, mode);
		} else {
			// Left partial word
			prof_apply_prim_mode_x(dp, left_mask, mode);
			dp++;

			// Middle full words
			for(i32 widx = start_w + 1; widx < end_w; widx++) {
				prof_apply_prim_mode_x(dp, 0xFFFFFFFFu, mode);
				dp++;
			}

			// Right partial word
			prof_apply_prim_mode_x(dp, right_mask, mode);
		}
	}
}

#if PROF_HISTORY
#define PROF_CSV_ROW_CAP (512 + PROF_HISTORY_SIZE * 12)
#else
#define PROF_CSV_ROW_CAP 512
#endif

static inline i32
prof_csv_append(char *row, i32 cap, i32 used, const char *fmt, ...)
{
	va_list args;
	int w;
	i32 left;

	used = clamp_i32(used, 0, cap - 1);
	left = cap - used;
	va_start(args, fmt);
	w = sys_vsnprintf(row + used, left, fmt, args);
	va_end(args);

	if(w < 0) {
		return used;
	}
	if(w >= left) {
		return cap - 1;
	}

	return used + w;
}

static inline void
prof_csv_push_row(struct alloc alloc, struct str8_list *list, char *row, i32 n, i32 cap)
{
	n      = clamp_i32(n, 0, cap - 1);
	row[n] = 0;
	str8_list_pushf(alloc, list, "%s\n", row);
}

static inline str8
prof_csv(struct alloc alloc, u32 max_records)
{
	char row[PROF_CSV_ROW_CAP];
	u32 record_count;
	i32 n;

	struct prof *prof                 = &PROFILER;
	struct str8_list list             = {0};
	struct prof_report_entry *entries = alloc_arr(alloc, entries, PROF_ANCHORS_SIZE);
	i32 precision                     = 4;
	u32 entry_count                   = 0;
	i32 cap                           = (i32)sizeof(row);

#if PROF_HISTORY
	u64 hist_sum[PROF_HISTORY_SIZE];
	i32 empty_n;
#endif

	if(!INT_TO_STRING[0][0]) {
		int_to_string_ini();
	}

	for(ssize i = 1; i < prof->anchor_count; ++i) {
		struct prof_anchor_hist *h = prof->anchors_hist + i;
		const char *label          = prof->anchor_labels[i];
		struct prof_report_entry *e;

		if(!label) { continue; }

		e                   = &entries[entry_count++];
		e->label            = label;
		e->zone             = (u16)i;
		e->hit_count        = h->hit_count.values[prof->smooth_slot];
		e->ms_inclusive     = h->us_inclusive.values[prof->smooth_slot] * 1e-3f;
		e->ms_exclusive     = h->us_exclusive.values[prof->smooth_slot] * 1e-3f;
		e->ms_inclusive_min = h->us_inclusive_min * 1e-3f;
		e->ms_inclusive_max = h->us_inclusive_max * 1e-3f;
	}

	qsort(entries, entry_count, sizeof(entries[0]), prof_report_sort_desc);

	record_count = entry_count;
	if(max_records != 0 && max_records < record_count) {
		record_count = max_records;
	}

	// TODO: make it a single path just zero out the unused fields.
#if PROF_HISTORY == PROF_HISTORY_FRAME
	str8_list_pushf(
		alloc,
		&list,
		"# history=%u size=%u history_idx=%u capture_n=%u update_idx=%u smooth=%u anchors=%u rows=%u\n",
		(u32)PROF_HISTORY,
		(u32)PROF_HISTORY_SIZE,
		(u32)prof->history_idx,
		prof->capture_n,
		prof->update_idx,
		(u32)prof->smooth_slot,
		(u32)prof->anchor_count,
		record_count);

#elif PROF_HISTORY && 0
	str8_list_pushf(
		alloc,
		&list,
		"# history=%u size=%u history_idx=%u update_idx=%u smooth=%u anchors=%u rows=%u\n",
		(u32)PROF_HISTORY,
		(u32)PROF_HISTORY_SIZE,
		(u32)prof->history_idx,
		prof->update_idx,
		(u32)prof->smooth_slot,
		(u32)prof->anchor_count,
		record_count);
#else
	str8_list_pushf(
		alloc,
		&list,
		"# history=0 size=0 update_idx=%u smooth=%u anchors=%u rows=%u\n",
		prof->update_idx,
		(u32)prof->smooth_slot,
		(u32)prof->anchor_count,
		record_count);
#endif

	n = sys_snprintf(row, cap, "zone,id,exclusive,inclusive,count,inclusive_min,inclusive_max");
#if PROF_HISTORY == PROF_HISTORY_FRAME
	n = prof_csv_append(row, cap, n, ",tot_exc,tot_inc,tot_hits");
#endif
#if PROF_HISTORY
	for(u16 i = 0; i < PROF_HISTORY_SIZE; ++i) {
		n = prof_csv_append(row, cap, n, ",%u", (u32)i);
	}
	mclr_array(hist_sum);
#endif
	prof_csv_push_row(alloc, &list, row, n, cap);

	for(u32 i = 0; i < record_count; ++i) {
		u8 buf[64];
		str8 str;
		struct prof_report_entry *r = &entries[i];

		if(!r->label) { continue; }

		n = sys_snprintf(row, cap, "%s,%u", r->label, (u32)r->zone);
		for(ssize j = 0; j < (ssize)ARRLEN(r->values); ++j) {
			str = prof_f32_to_str8(buf, r->values[j], j == 2 ? 2 : precision);
			n   = prof_csv_append(row, cap, n, ",%.*s", str8_spread(str));
		}
#if PROF_HISTORY == PROF_HISTORY_FRAME
		{
			struct prof_anchor_totals *t = &prof->totals[r->zone];
			n                            = prof_csv_append(row, cap, n, ",%" PRIu64 ",%" PRIu64 ",%" PRIu64, t->sum_exc, t->sum_inc, t->sum_hits);
		}
#endif
#if PROF_HISTORY
		for(u16 h = 0; h < PROF_HISTORY_SIZE; ++h) {
			u32 excl = prof_zone_excl_at(r->zone, h);
			hist_sum[h] += excl;
			n = prof_csv_append(row, cap, n, ",%u", excl);
		}
#endif
		prof_csv_push_row(alloc, &list, row, n, cap);
	}

#if PROF_HISTORY
	empty_n = 6; // id + 5 ema columns
#if PROF_HISTORY == PROF_HISTORY_FRAME
	empty_n += 3; // tot_*
	n = sys_snprintf(row, cap, "__dt");
	for(i32 e = 0; e < empty_n; ++e) {
		n = prof_csv_append(row, cap, n, ",");
	}
	for(u16 h = 0; h < PROF_HISTORY_SIZE; ++h) {
		const struct prof_hist_slot *slot = prof_history_logical_at(h);
		u32 dt                            = slot ? slot->dt_us : 0;
		n                                 = prof_csv_append(row, cap, n, ",%u", dt);
	}
	prof_csv_push_row(alloc, &list, row, n, cap);
#endif
	n = sys_snprintf(row, cap, "__sum");
	for(i32 e = 0; e < empty_n; ++e) {
		n = prof_csv_append(row, cap, n, ",");
	}
	for(u16 h = 0; h < PROF_HISTORY_SIZE; ++h) {
		n = prof_csv_append(row, cap, n, ",%" PRIu64, hist_sum[h]);
	}
	prof_csv_push_row(alloc, &list, row, n, cap);
#endif

	return str8_list_join(alloc, &list, NULL);
}

static b32
prof_csv_save(struct alloc alloc, str8 app_name, str8 app_org)
{
	b32 res                    = false;
	struct date_time date_time = date_time_from_epoch_2000_gmt(sys_epoch_2000(NULL));
	str8 csv                   = prof_csv(alloc, 0);
	str8 path                  = str8_fmt_push(
		alloc,
		"%.*s-%04d-%02d-%02d_%02d:%02d:%02d-prof.csv",
		str8_spread(app_name),
		date_time.year,
		date_time.month,
		date_time.day,
		date_time.hour,
		date_time.min,
		date_time.sec);
	str8 full_path = sys_path_to_data_path(alloc, path, app_org, app_name);
	void *f        = sys_file_open_w(full_path);

	if(f == NULL) {
		log_error("prof", "csv save failed: %s", full_path.str);
		return res;
	}
	if(csv.size > 0) {
		sys_file_w(f, csv.str, (u32)csv.size);
	}
	sys_file_close(f);
	res = true;
	log_info("prof", "prof csv saved: %s", full_path.str);
	return res;
}

#else

static inline void
prof_ini(void)
{
}

static inline void
prof_sort_set(enum prof_sort sort)
{
}

static inline enum prof_sort
prof_sort_get(void)
{
	return PROF_SORT_EXCLUSIVE;
}

static inline void
prof_smooth_set(enum prof_smooth smooth)
{
}

static inline void
prof_block_start(const char *label, ssize idx)
{
}

static inline void
prof_block_end_internal(void)
{
}

static inline void
prof_upd(b32 record_data)
{
}

static inline void
prof_drw(
	struct prof_report *report,
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
}

static inline str8
prof_csv(struct alloc alloc, u32 max_records)
{
	return str8_lit("");
}

static b32
prof_csv_save(struct alloc alloc, str8 app_name, str8 app_org)
{ return false; }

#endif

#if !PROF_HISTORY
static inline u16
prof_history_capacity(void)
{
	return 0;
}

static inline void
prof_graph_drw(
	struct prof_report *report,
	struct gfx_ctx ctx,
	i32 sx,
	i32 sy,
	i32 x_scale,
	i32 y_scale,
	i32 height,
	i32 line_spacing)
{
}
#endif

#if PROF_HISTORY != PROF_HISTORY_FRAME
static inline const struct prof_hist_slot *
prof_history_logical_at(u16 logical_i)
{
	(void)logical_i;
	return 0;
}

static inline u32
prof_capture_n(void)
{
	return 0;
}
#endif
