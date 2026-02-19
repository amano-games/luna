#pragma once

#include "base/dbg.h"
#include "base/str.h"
#include "base/types.h"
#include "base/utils.h"
#include "sys/sys.h"

#define PROF_ANCHOR_SIZE 4096

struct prof_block {
	str8 label;
	// Total time this block was on the stack.
	u32 us_total;

	// Total time this block was at the top of the stack.
	u32 us_exclusive;

	// Number of frames for this block currently on the stack.
	u32 frame_count;

	// Total number of times we entered this block.
	u32 hit_count;
};

struct prof_frame {
	struct prof_block *block;
	u32 us_start;
	u32 child_time;
};

struct prof {
	u32 us_start;
	u32 us_end;
	struct prof_block blocks[PROF_ANCHOR_SIZE];
	struct prof_frame frames[64];
	ssize block_count;
	ssize frame_count;
};

static struct prof PROFILER;

#define prof_block(name) prof_block_start(name, __COUNTER__ + 1)
/* #define prof_block(name) prof_block_start(name, \
 	({ static int i = -1; if (i == -1) i = prof_next_block_idx(); i; })) */

#define prof_block_func() prof_block(__func__)

static void
prof_ini(void)
{
	struct prof *prof = &PROFILER;
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
	dbg_assert(prof->block_count < (ssize)ARRLEN(prof->blocks));
	return prof->block_count++;
}

void
prof_block_start(str8 label, ssize block_idx)
{
	struct prof *prof = &PROFILER;
	dbg_assert(block_idx < (ssize)ARRLEN(prof->blocks));
	struct prof_block *block = prof->blocks + block_idx;
	block->label             = label;
	block->hit_count++;
	block->frame_count++;
	dbg_assert(prof->frame_count < (ssize)ARRLEN(prof->frames));
	prof->frames[prof->frame_count++] = (struct prof_frame){
		.us_start = sys_time_us(),
		.block    = block,
	};
}

void
prof_block_end(void)
{
	u32 now_us        = sys_time_us();
	struct prof *prof = &PROFILER;
	if(prof->frame_count == 0) { return; } // if we just initialized in the middle a profiler then ignore this block
	struct prof_frame *frame = &prof->frames[--prof->frame_count];
	u32 time_total           = now_us - frame->us_start;
	u32 time_exclusive       = time_total - frame->child_time;

	// Add this frame's run time to the child time of its parent frame.
	if(prof->frame_count) {
		prof->frames[prof->frame_count - 1].child_time += time_total;
	}

	frame->block->us_exclusive += time_exclusive;
	frame->block->frame_count--; // us_total counts only outermost invocations (collapses recursion)

	// Update the block's total time, unless it still has another frame on the
	// stack.
	if(frame->block->frame_count == 0) {
		frame->block->us_total += time_total;
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

	for(size_t i = 0; i < ARRLEN(prof->blocks); i++) {
		struct prof_block *block = prof->blocks + i;

		if(block->label.size == 0 || block->hit_count == 0) { continue; }
		dbg_assert(block->frame_count == 0);

		f32 percent  = total_time ? (100.0f * (f32)block->us_exclusive / (f32)total_time) : 0.0f;
		f32 avg_excl = (f32)block->us_exclusive / (f32)block->hit_count;
		acc_perc += percent;

		str8_list_pushf(
			alloc,
			&list,
			"%-12.12s %7" PRIu32 " %9" PRIu32 " %9" PRIu32 " %9.2f %5.1f%%",
			block->label.str,
			block->hit_count,
			block->us_total,
			block->us_exclusive,
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
