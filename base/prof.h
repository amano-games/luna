#pragma once

#include "base/dbg.h"
#include "base/str.h"
#include "base/types.h"
#include "base/utils.h"
#include "sys/sys.h"

#define PROF_ANCHOR_SIZE 4096

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

	struct prof_anchor anchors[PROF_ANCHOR_SIZE];
	struct prof_frame frames[64];
	ssize anchor_count;
	ssize frame_count;
};

static struct prof PROFILER;

#define prof_stringize_2(x)    #x
#define prof_stringize(x)      prof_stringize_2(x)
#define prof_unique_name(name) name prof_stringize(__LINE__)

#define prof_block(name) prof_block_start(prof_unique_name(name), __COUNTER__ + 1)
/* #define prof_block(name) prof_block_start(name, \
 	({ static int i = -1; if (i == -1) i = prof_next_block_idx(); i; })) */

#define prof_block_func() prof_block(__func__)

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

static ssize
prof_next_block_idx(void)
{
	struct prof *prof = &PROFILER;
	dbg_assert(prof->anchor_count < (ssize)ARRLEN(prof->anchors));
	return prof->anchor_count++;
}

void
prof_block_start(const char *label, ssize idx)
{
	struct prof *prof = &PROFILER;
	dbg_assert(idx < (ssize)ARRLEN(prof->anchors));

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
prof_block_end(void)
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
