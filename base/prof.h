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

// Reserved indices for luna sys TU blocks. App call sites allocate from
// PROF_ANCHOR_SYS_NUM_COUNT upward so the shared PROFILER is not corrupted
// across the two compilation units.
enum prof_anchor_sys {
	PROF_ANCHOR_SYS_NONE,

	PROF_ANCHOR_SYS_UPD,
	PROF_ANCHOR_SYS_DRW,

	PROF_ANCHOR_SYS_NUM_COUNT,
};

#if defined(PROF)

#define PROF_HISTORY_SIZE        1     // number of frames of history to keep
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

#define PROF_ANCHORS_SIZE 2
#define PROF_HISTORY_SIZE 2
#define PROF_FRAMES_SIZE  2
#define prof_block(...)
#define prof_block_func(...)
#define prof_block_end(...)
#endif

#define PROF_TRACKER_HISTORTY_SLOTS  3
#define PROF_THROWAWAY_UPDATES_COUNT 3

struct prof_hist_scalar {
	f32 values[PROF_TRACKER_HISTORTY_SLOTS];
	f32 variances[PROF_TRACKER_HISTORTY_SLOTS];
};

struct prof_anchor {
	u32 us_exclusive; // Does not include children
	u32 us_inclusive; // Does include children

	u32 hit_count;
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

	u32 child_time;
	const char *label;
};

struct prof {
	u16 parent_idx;
	u16 smooth_slot;

	u32 update_idx; // 2^31 at 100fps = 280 days
	u32 last_upd_us;
	u32 frame_dt_us;
	u32 frame_sum_us;

	const char *anchor_labels[PROF_ANCHORS_SIZE];
	struct prof_anchor anchors[PROF_ANCHORS_SIZE];
	struct prof_anchor_hist anchors_hist[PROF_ANCHORS_SIZE];
	struct prof_frame frames[PROF_FRAMES_SIZE];

	struct prof_hist_scalar frame_time;

	u16 anchor_count;
	u16 frame_count;
};

#define PROF_REPORT_NUM_VALUES 5
#define PROF_REPORT_NUM_TITLES 3
#define PROF_REPORT_NUM_HEADER (PROF_REPORT_NUM_VALUES + 1)

struct prof_report_entry {
	const char *label;
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

// Title is: 32.180 ms/frame (fps: 31.14) sort self - current time
// Header is zone self hier count
struct prof_report {
	str8 titles[PROF_REPORT_NUM_TITLES];
	str8 headers[PROF_REPORT_NUM_HEADER];
	u32 entry_count;
	struct prof_report_entry entries[PROF_ANCHORS_SIZE];
};

static f32 PROF_TIMES_TO_REACH_90_PERCENT[PROF_TRACKER_HISTORTY_SLOTS];
static f32 PROF_PRECOMPUTED_FACTORS[PROF_TRACKER_HISTORTY_SLOTS];

// Defined once in sys.c — must be shared across luna/game TUs.
extern struct prof PROFILER;

static char INT_TO_STRING[100][4];
static char INT_TO_STRING_DECIMAL[100][4];
static char INT_TO_STRING_MID_DECIMAL[100][4];
static str8 INT_TO_STR8[100];
static str8 INT_TO_STR8_DECIMAL[100];
static str8 INT_TO_STR8_MID_DECIMAL[100];

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

#if defined(PROF)
static inline ssize
prof_next_block_idx(void)
{
	static ssize next = PROF_ANCHOR_SYS_NUM_COUNT;
	dbg_assert(next < (ssize)PROF_ANCHORS_SIZE);
	return next++;
}
#endif

static inline void
prof_ini(void)
{
	struct prof *prof = &PROFILER;
	dbg_assert(ARRLEN(prof->anchors) < U16_MAX);
	dbg_assert(ARRLEN(prof->frames) < U16_MAX);

	{
		PROF_TIMES_TO_REACH_90_PERCENT[0] = 0.1f;
		PROF_TIMES_TO_REACH_90_PERCENT[1] = 0.8f;
		PROF_TIMES_TO_REACH_90_PERCENT[2] = 2.5f;
	}
	for(ssize i = 0; i < (ssize)ARRLEN(prof->frame_time.values); ++i) {
		prof->frame_time.values[i] = sys_dt_us_target_get();
	}
}

static inline void
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
prof_report_sort_inclusive_desc(const void *a, const void *b)
{
	const struct prof_report_entry *aa = a;
	const struct prof_report_entry *bb = b;
	f32 a_value                        = aa->ms_exclusive;
	f32 b_value                        = bb->ms_exclusive;

	if(b_value > a_value) return 1;
	if(b_value < a_value) return -1;
	return 0;
}

static inline struct prof_report *
prof_report_create(struct alloc alloc)
{
	struct prof_report *res = alloc_struct(alloc, res);
	struct prof *prof       = &PROFILER;
	prof->smooth_slot       = 2; // TODO: convert to enum

	if(!INT_TO_STRING[0][0])
		int_to_string_ini();

	u32 now = sys_time_us();
	f32 fps = 0;
	f32 ms  = 0;

	{
		f32 avg_frame_time_us = prof->frame_time.values[prof->smooth_slot];

		if(avg_frame_time_us == 0) {
			avg_frame_time_us = sys_dt_us_target_get();
		}
		fps = 1000000.0f / avg_frame_time_us;
		ms  = avg_frame_time_us * 1e-3f;
	}

	res->entry_count = 0;
	for(ssize i = 1; i < prof->anchor_count; ++i) {
		struct prof_anchor *a       = prof->anchors + i;
		struct prof_anchor_hist *h  = prof->anchors_hist + i;
		const char *label           = prof->anchor_labels[i];
		struct prof_report_entry *e = &res->entries[res->entry_count++];
		e->label                    = label;
		e->hit_count                = h->hit_count.values[prof->smooth_slot];
		e->ms_inclusive             = h->us_inclusive.values[prof->smooth_slot] * 1e-3f;
		e->ms_exclusive             = h->us_exclusive.values[prof->smooth_slot] * 1e-3f;
		e->ms_inclusive_min         = h->us_inclusive_min * 1e-3f;
		e->ms_inclusive_max         = h->us_inclusive_max * 1e-3f;
	}

	/* ------------------------------------------------------------
	   Sort
	------------------------------------------------------------ */
	qsort(res->entries,
		res->entry_count,
		sizeof(struct prof_report_entry),
		prof_report_sort_inclusive_desc);

	{
		u8 fps_buf[32];
		u8 ms_buf[32];
		str8 fps_str   = prof_f32_to_str8(fps_buf, fps, 3);
		str8 ms_str    = prof_f32_to_str8(ms_buf, ms, 2);
		res->titles[0] = str8_fmt_push(alloc, "%s ms/frame fps: %s", ms_str.str, fps_str.str);
		res->titles[1] = str8_lit("");
		res->titles[2] = str8_lit("");
		// TODO: Split ms/frame, fps?
		// TODO: Sort should be configurable
		// res->titles[1] = str8_lit("sort exclusive - current frame");

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
	u32 now_us        = sys_time_us();
	u32 dt_us         = 0;

	if(prof->update_idx == 0) {
		dt_us = sys_dt_us_target_get();
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
			}
		}

		{
			// Update frame time
			if(prof->update_idx < PROF_THROWAWAY_UPDATES_COUNT) {
				// Avoid smoothing when the profiler is starting up
				prof_history_scalar_eternity(&prof->frame_time, dt_us);
			} else {
				prof_history_scalar_upd(&prof->frame_time, dt_us, PROF_PRECOMPUTED_FACTORS);
			}
		}

		++prof->update_idx;
		// history_index = (history_index + 1) % NUM_FRAME_SLOTS;
	}

	if(prof->anchor_count > 0) {
		mclr(prof->anchors + 1, (prof->anchor_count - 1) * sizeof(prof->anchors[0]));
	}
}

static inline void
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
	prof_block("prof_drw");
	i32 pad                    = 1;
	struct prof_report *report = prof_report_create(alloc);
	i32 field_width            = txt_width(str8_lit("5555.55"));
	i32 max_columns            = 3;
	i32 name_width             = full_width - (field_width * max_columns);
	precision                  = clamp_i32(precision, 1, 4);

#if 1
	for(ssize i = 0; i < (ssize)ARRLEN(report->titles); ++i) {
		if(report->titles[i].size > 0) {
			i32 title_x0 = sx;
			i32 title_x1 = title_x0 + full_width;
			prof_rec_fill(ctx, title_x0, sy, full_width, line_spacing + pad, PRIM_MODE_BLACK);

			txt_drw(sx + pad, sy + pad, report->titles[i], SPR_MODE_WHITE);

			sy += line_spacing;
			height -= abs_i32(line_spacing);
		}
	}
#endif

	i32 max_records  = height / abs_i32(line_spacing);
	i32 record_count = min_i32(report->entry_count, max_records);

#if 1
	prof_rec_fill(ctx, sx, sy, full_width, line_spacing, PRIM_MODE_WHITE);
	if(report->headers[0].size > 0) {
		txt_drw(sx + pad, sy + pad, report->headers[0], SPR_MODE_BLACK);
	}

	for(ssize j = 1; j < max_columns + 1; ++j) {
		if(report->headers[j].size > 0) {
			i32 col_x = sx + name_width + pad + field_width * (j - 1);
			txt_drw(col_x, sy + pad, report->headers[j], SPR_MODE_BLACK);
		}
	}
	sy += line_spacing;
#endif

	// Draw bg
	prof_rec_fill(ctx, sx, sy, full_width, (line_spacing * record_count) + (pad * (record_count - 1)), PRIM_MODE_BLACK);
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
	prof_block_end();
}

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

struct prof_span_blit {
	u32 *dp;  //pixel
	u16 dmax; // count of dst words -1
	u16 dadd;
	u32 ml;   // boundary mask left
	u32 mr;   // boundary mask right
	i16 mode; // drawing mode
	i16 doff; // bitoffset of first dst bit
	u16 dst_wword;
	i16 y;
};

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
	u32 left_mask = 0xFFFFFFFFu >> start_bit;

	// right mask
	u32 right_mask = 0xFFFFFFFFu << (31 - end_bit);

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

static inline str8
prof_csv(struct alloc alloc, u32 max_records)
{
	str8 res                   = {0};
	str8 delimiter             = str8_lit(",");
	struct prof_report *report = prof_report_create(alloc);
	u32 record_count           = min_i32(max_records, report->entry_count);
	struct str8_list list      = {0};
	str8 headers[]             = {
		str8_lit("zone"),
		str8_lit("exlusive"),
		str8_lit("inclusive"),
		str8_lit("count"),
		str8_lit("inclusive_min"),
		str8_lit("inclusive_max"),
	};

	for(ssize i = 0; i < (ssize)ARRLEN(headers); ++i) {
		if(i > 0) {
			str8_list_push(alloc, &list, delimiter);
		}
		str8_list_push(alloc, &list, headers[i]);
	}

	str8_list_push(alloc, &list, str8_lit("\n"));

	i32 precision = 4;
	for(u32 i = 0; i < record_count; ++i) {
		u8 buf[64];
		str8 str;
		struct prof_report_entry *r = &report->entries[i];
		if(!r->label) { continue; }

		if(i > 0) {
			str8_list_push(alloc, &list, str8_lit("\n"));
		}

		str8_list_push(alloc, &list, str8_cstr((char *)r->label));
		str8_list_push(alloc, &list, delimiter);

		for(ssize j = 0; j < (ssize)ARRLEN(r->values); ++j) {
			if(j > 0) {
				str8_list_push(alloc, &list, delimiter);
			}
			str = prof_f32_to_str8(buf, r->values[j], j == 2 ? 2 : precision);
			str8_list_push(alloc, &list, str8_cpy_push(alloc, str));
		}
	}
	res = str8_list_join(alloc, &list, NULL);

	return res;
}
