#pragma once

#include "base/dbg.h"
#include "base/types.h"
#include "base/utils.h"
#include "sys/sys.h"

#if defined(TRACE_AUTO)
#define SPALL_AUTO_IMPLEMENTATION
#include "spall_native_auto.h"
#endif

#if defined(TRACE)
#define PROF_ANCHOR_SIZE 4096
#else
#define PROF_ANCHOR_SIZE 4096
#endif

struct prof_block {
	char const *label;
	// Total time this block was on the stack.
	u32 time_total;

	// Total time this block was at the top of the stack.
	u32 time_exclusive;

	// Number of frames for this block currently on the stack.
	u32 frame_count;

	// Total number of times we entered this block.
	u32 hit_count;

	// Total number of bytes this block processed.
	u32 byte_count;
};

struct prof_frame {
	struct prof_block *block;
	f32 tsc_start;
	u32 child_time;
};

struct prof {
	u32 tsc_start;
	u32 tsc_end;
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
	prof->frames[prof->frame_count++] = (struct prof_frame){
		.tsc_start = sys_time_elapsed(),
		.block     = block,
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
	u32 nowtsc        = sys_time_us();
	struct prof *prof = &PROFILER;
	dbg_assert(prof->frame_count);
	struct prof_frame *frame = &prof->frames[--prof->frame_count];
	u32 time_total           = nowtsc - frame->tsc_start;
	u32 time_exclusive       = time_total - frame->child_time;

	// Add this frame's run time to the child time of its parent frame.
	if(prof->frame_count) {
		prof->frames[prof->frame_count - 1].child_time += time_total;
	}

	frame->block->time_exclusive += time_exclusive;
	frame->block->frame_count--;

	// Update the block's total time, unless it still has another frame on the
	// stack.
	if(frame->block->frame_count == 0) {
		frame->block->time_total += time_total;
	}
}

void
prof_end(void)
{
	struct prof *prof = &PROFILER;
	prof->tsc_end     = sys_time_us();
}

void trace_ini(str8 file_name, u8 *buffer, usize size);
void trace_close(void);

#if defined(TRACE)
/* #define TRACE_START(s) spall_buffer_begin( \
 	&SPALL_CTX, \
 	&SPALL_BUFFER, \
 	s, \
 	sizeof(s) - 1, \
 	sys_time_us())

 #define TRACE_END() spall_buffer_end( \
 	&SPALL_CTX, \
 	&SPALL_BUFFER, \
 	sys_time_us())
*/
#define TRACE_START(...)
#define TRACE_END(...)
#else
#define TRACE_START(...)
#define TRACE_END(...)
#endif
