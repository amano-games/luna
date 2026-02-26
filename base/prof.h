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

// #if !defined(PROF)
// #define PROF
// #endif
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

#define PROF_TRACKER_HISTORTY_SLOTS  3
#define PROF_THROWAWAY_UPDATES_COUNT 3

struct prof_history_scalar {
	f32 values[PROF_TRACKER_HISTORTY_SLOTS];
	f32 variances[PROF_TRACKER_HISTORTY_SLOTS];
};

struct prof_anchor {
	u32 us_exclusive; // Does not include children
	u32 us_inclusive; // Does include children

	u32 hit_count;

	struct prof_history_scalar excl_hist;
	struct prof_history_scalar incl_hist;
	struct prof_history_scalar hit_hist;

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
	u8 smooth_slot;

	u32 update_idx; // 2^31 at 100fps = 280 days
	u32 last_upd_us;
	u32 frame_dt_us;
	u32 frame_sum_us;

	struct prof_anchor anchors[PROF_ANCHORS_SIZE];
	struct prof_frame frames[PROF_FRAMES_SIZE];

	u32 history_idx;
	// u32 history[PROF_ANCHORS_SIZE][PROF_HISTORY_SIZE]; // 256K

	struct prof_history_scalar frame_time;

	ssize anchor_count;
	ssize frame_count;
};

static f32 PROF_TIMES_TO_REACH_90_PERCENT[PROF_TRACKER_HISTORTY_SLOTS];
static f32 PROF_PRECOMPUTED_FACTORS[PROF_TRACKER_HISTORTY_SLOTS];

static struct prof PROFILER;

static char INT_TO_STRING[100][4];
static char INT_TO_STRING_DECIMAL[100][4];
static char INT_TO_STRING_MID_DECIMAL[100][4];
static str8 INT_TO_STR8[100];
static str8 INT_TO_STR8_DECIMAL[100];
static str8 INT_TO_STR8_MID_DECIMAL[100];

static void
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

static inline void prof_history_scalar_upd(struct prof_history_scalar *h, f32 sample, f32 *factors);
static inline void prof_history_scalar_eternity(struct prof_history_scalar *h, f32 new_value);

static void
prof_ini(void)
{
	struct prof *prof = &PROFILER;
	dbg_assert(ARRLEN(prof->anchors) < U16_MAX);
	dbg_assert(ARRLEN(prof->frames) < U16_MAX);
	mclr_struct(prof);

	{
		PROF_TIMES_TO_REACH_90_PERCENT[0] = 0.1f;
		PROF_TIMES_TO_REACH_90_PERCENT[1] = 0.8f;
		PROF_TIMES_TO_REACH_90_PERCENT[2] = 2.5f;
	}
	for(ssize i = 0; i < (ssize)ARRLEN(prof->frame_time.values); ++i) {
		prof->frame_time.values[i] = SYS_UPS_DT_US;
	}
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
#define PROF_REPORT_NUM_TITLES 3
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

	if(!INT_TO_STRING[0][0])
		int_to_string_ini();

	u32 now = sys_time_us();

	for(ssize i = 0; i < prof->anchor_count; ++i) {
		struct prof_anchor *a = prof->anchors + i;

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

	// TODO: convert to enum
	prof->smooth_slot     = 2;
	f32 avg_frame_time_us = prof->frame_time.values[prof->smooth_slot];

	if(avg_frame_time_us == 0) {
		avg_frame_time_us = SYS_UPS_DT_US;
	}
	f32 fps = 1000000.0f / avg_frame_time_us;
	f32 ms  = avg_frame_time_us * 1e-3f;
	u8 fps_buf[32];
	u8 ms_buf[32];
	str8 fps_str = prof_f32_to_str8(fps_buf, fps, 3);
	str8 ms_str  = prof_f32_to_str8(ms_buf, ms, 2);

	res->titles[0] = str8_fmt_push(alloc, "%s ms/frame (fps: %s)", ms_str.str, fps_str.str);
	// TODO: Split ms/frame, fps?
	// TODO: Sort should be configurable
	// res->titles[1] = str8_lit("sort exclusive - current frame");

	res->headers[0] = str8_lit("zone");
	res->headers[1] = str8_lit("excl");
	res->headers[2] = str8_lit("incl");
	res->headers[3] = str8_lit("count");

	return res;
}

void
prof_upd(b32 record_data)
{
	struct prof *prof = &PROFILER;
	u32 now_us        = sys_time_us();
	u32 dt_us         = 0;

	if(prof->update_idx == 0) {
		dt_us = SYS_UPS_DT_US;
	} else {
		dt_us = now_us - prof->last_upd_us;
		if(dt_us == 0) {
			dt_us = 1;
		}
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
			for(ssize i = 0; i < prof->anchor_count; ++i) {
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

	if(prof->update_idx < PROF_THROWAWAY_UPDATES_COUNT) {
		// Avoid smoothing when the profiler is starting up
		prof_history_scalar_eternity(&prof->frame_time, dt_us);
	} else {
		prof_history_scalar_upd(
			&prof->frame_time,
			dt_us,
			PROF_PRECOMPUTED_FACTORS);
	}

	for(ssize i = 0; i < prof->anchor_count; ++i) {
		struct prof_anchor *a = &prof->anchors[i];

		if(!a->label) continue;

		if(prof->update_idx < PROF_THROWAWAY_UPDATES_COUNT) {
			prof_history_scalar_eternity(&a->excl_hist, (f32)a->us_exclusive);
			prof_history_scalar_eternity(&a->incl_hist, (f32)a->us_inclusive);
			prof_history_scalar_eternity(&a->hit_hist, (f32)a->hit_count);
		} else {
			prof_history_scalar_upd(&a->excl_hist, (f32)a->us_exclusive, PROF_PRECOMPUTED_FACTORS);
			prof_history_scalar_upd(&a->incl_hist, (f32)a->us_inclusive, PROF_PRECOMPUTED_FACTORS);
			prof_history_scalar_upd(&a->hit_hist, (f32)a->hit_count, PROF_PRECOMPUTED_FACTORS);
		}
	}

	if(!record_data) {
		for(ssize i = 0; i < prof->anchor_count; ++i) {
			prof->anchors[i].us_exclusive = 0;
			prof->anchors[i].us_inclusive = 0;
			prof->anchors[i].hit_count    = 0;
		}
		return;
	}

	for(ssize i = 0; i < prof->anchor_count; ++i) {
		prof->anchors[i].us_exclusive = 0;
		prof->anchors[i].us_inclusive = 0;
		prof->anchors[i].hit_count    = 0;
	}

	++prof->update_idx;
	// prof->history_idx = (prof->history_idx + 1) % ARRLEN(prof->history);
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
	i32 pad                    = 1;
	struct prof_report *report = prof_report_create(alloc);
	i32 field_width            = txt_width(str8_lit("5555.55"));
	i32 name_width             = full_width - (field_width * (ARRLEN(report->headers) - 1));
	precision                  = clamp_i32(precision, 1, 4);

#if 1
	for(ssize i = 0; i < (ssize)ARRLEN(report->titles); ++i) {
		if(report->titles[i].size > 0) {
			i32 title_x0 = sx;
			i32 title_x1 = title_x0 + full_width;
			gfx_rec_fill(ctx, title_x0, sy, full_width, line_spacing + pad, PRIM_MODE_BLACK);

			txt_drw(sx + pad, sy + pad, report->titles[i], SPR_MODE_WHITE);

			sy += line_spacing;
			height -= abs_i32(line_spacing);
		}
	}
#endif

	i32 max_records  = height / abs_i32(line_spacing);
	i32 record_count = min_i32(report->entry_count, max_records);

#if 1
	gfx_rec_fill(ctx, sx, sy, full_width, line_spacing, PRIM_MODE_WHITE);
	if(report->headers[0].size > 0) {
		txt_drw(sx + pad, sy + pad, report->headers[0], SPR_MODE_BLACK);
	}

	for(ssize j = 1; j < (ssize)ARRLEN(report->headers); ++j) {
		if(report->headers[j].size > 0) {
			i32 col_x = sx + name_width + pad + field_width * (j - 1);
			txt_drw(col_x, sy + pad, report->headers[j], SPR_MODE_BLACK);
		}
	}
	sy += line_spacing;
#endif

	// Draw bg
	gfx_rec_fill(ctx, sx, sy, full_width, (line_spacing * record_count) + (pad * (record_count - 1)), PRIM_MODE_BLACK);
	for(ssize i = 0; i < record_count; ++i) {
		u8 buf[128];
		str8 str;
		struct prof_report_entry *r = &report->entries[i];
		i32 x                       = sx;

		txt_drw(x + 1, sy, r->label, SPR_MODE_WHITE);
		for(ssize j = 0; j < (ssize)ARRLEN(r->values); ++j) {
			if(j == 2) {
				// hit count stays integer
				str.size = sys_sprintf((char *)buf, "%" PRIu32, r->values[j]);
				str.str  = buf;
			} else {
				str = prof_f32_to_str8(buf, r->values[j], precision);
			}

			txt_drw(sx + pad + name_width + field_width * j, sy + pad, str, SPR_MODE_WHITE);
		}

		sy += line_spacing + pad;
	}
}

static inline void
prof_history_scalar_eternity(struct prof_history_scalar *h, f32 new_value)
{
	f32 new_variance = new_value * new_value;
	for(ssize i = 1; i < (ssize)ARRLEN(h->variances); ++i) {
		h->values[i]    = new_value;
		h->variances[i] = new_variance;
	}

	// TODO: history
}

static inline void
prof_history_scalar_upd(struct prof_history_scalar *h, f32 sample, f32 *factors)
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

		str8_cpy(&INT_TO_STR8[x], &res);
		str8_cat_in_place(&res, &INT_TO_STR8_DECIMAL[y]);
		return res;
	}
	case 3: {
		if(value < 0 || value >= 10) break;
		value *= 10.0f;

		x = value;
		y = (value - x) * 100;

		str8 b = INT_TO_STR8_DECIMAL[y];
		b.str += 1;
		b.size -= 1;
		str8_cpy(&INT_TO_STR8[x], &res);
		str8_cat_in_place(&res, &b);
		return res;
	}
	case 4: {
		if(value < 0 || value >= 1) break;

		value *= 100;
		x = value;
		y = (value - x) * 100;

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
