#pragma once

#include "base/dbg.h"
#include "base/str.h"
#include "base/types.h"
#include "base/utils.h"
#include "sys/sys.h"

#define PROF_ANCHOR_SIZE 4096

struct prof_block {
	char const *label;
	// Total time this block was on the stack.
	u32 us_total;

	// Total time this block was at the top of the stack.
	u32 us_exclusive;

	// Number of frames for this block currently on the stack.
	u32 frame_count;

	// Total number of times we entered this block.
	u32 hit_count;

	// Total number of bytes this block processed.
	u32 byte_count;
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

#define prof_block(name) prof_block_start(name, __COUNTER__)
/* #define prof_block(name) prof_block_start(name, \
 	({ static int i = -1; if (i == -1) i = prof_next_block_idx(); i; })) */

#define prof_func() prof_block(__func__)

static void
prof_init(void)
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
prof_block_start(const char *label, ssize block_idx)
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
prof_record_bytes(ssize bytes)
{
	struct prof *prof = &PROFILER;
	dbg_assert(prof->frame_count);
	prof->frames[prof->frame_count - 1].block->byte_count += bytes;
}

void
prof_block_end(void)
{
	u32 now_us        = sys_time_us();
	struct prof *prof = &PROFILER;
	dbg_assert(prof->frame_count);
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

	if(!prof->us_end) { prof_close(); }

	u32 total_time = prof->us_end - prof->us_start;
	str8_list_pushf(alloc, &list, "%-18s %8s %12s %12s %6s", "Block", "Hits", "Total(us)", "Excl(us)", "%");
	str8_list_pushf(alloc, &list, "%.*s", 60, "------------------------------------------------------------");

	dbg_assert(prof->frame_count == 0);
	for(size_t i = 0; i < ARRLEN(prof->blocks); i++) {
		struct prof_block *block = prof->blocks + i;
		dbg_assert(block->frame_count == 0);
		if(block->label == NULL) { continue; }
		f32 percent = total_time ? (100.0f * (f32)block->us_exclusive / (f32)total_time) : 0.0f;

		str8_list_pushf(
			alloc,
			&list,
			"%-18s %8" PRIu32 " %12" PRIu32 " %12" PRIu32 " %5.1f%%",
			block->label,
			block->hit_count,
			block->us_total,
			block->us_exclusive,
			(double)percent);

		if(block->byte_count) {
			// sys_printf(" %4.2f GiB/s", calculate_gib_per_s(block->byte_count, block->total_time));
		}
	}

	str8_list_pushf(alloc, &list, "%.*s", 60, "------------------------------------------------------------");
	str8_list_pushf(
		alloc,
		&list,
		"%-18s %8s %12" PRIu32 " %12s %6s",
		"Total",
		"",
		total_time,
		"",
		"");

	struct str_join params = {.sep = str8_lit("\n")};
	return str8_list_join(alloc, &list, &params);
}
