#pragma once

#include "base/types.h"
#include "base/utils.h"
#include <stdalign.h>

#define MKILOBYTE(value) ((value) * 1024LL)
#define MMEGABYTE(value) (MKILOBYTE(value) * 1024LL)
#define MGIGABYTE(value) (MMEGABYTE(value) * 1024LL)
#define MTERABYTE(value) (MGIGABYTE(value) * 1024LL)

// used for user defined allocations
// alloc(ctx, size) -> ctx: pointer to some memory manager
struct alloc {
	void *(*allocf)(void *ctx, ssize size, ssize align);
	void *ctx;
};

struct alloc_meta {
	const char *file;
	int line;
	const char *tag; // optional grouping label
	ssize size;
	void *ptr;
};

#define alloc_struct(alloc, ptr)     (__typeof__(ptr))alloc_size(alloc, sizeof(*(ptr)), alignof(__typeof__(*(ptr))), false)
#define alloc_struct_clr(alloc, ptr) (__typeof__(ptr))alloc_size(alloc, sizeof(*(ptr)), alignof(__typeof__(*(ptr))), true)
#define alloc_arr(alloc, ptr, count) \
	(__typeof__(ptr))alloc_size((alloc), sizeof(*(ptr)) * (count), alignof(__typeof__(*(ptr))), false)
#define alloc_arr_clr(alloc, ptr, count) \
	(__typeof__(ptr))alloc_size((alloc), sizeof(*(ptr)) * (count), alignof(__typeof__(*(ptr))), true)

#if defined(DEBUG)
#define DBG_ALLOCS_MAX 65536

static struct alloc_meta DBG_MEM_ALLOCS[DBG_ALLOCS_MAX];
static ssize DBG_MEM_ALLOCS_COUNT = 0;

#define alloc_size(a, size, align, clr) \
	alloc_size_dbg((a), (size), (align), (clr), __FILE__, __LINE__, NULL)

static inline void *
alloc_size_dbg(struct alloc alloc, ssize mem_size, ssize align, b32 clr, const char *file, int line, const char *tag)
{
	void *mem = alloc.allocf(alloc.ctx, mem_size, align);
	if(clr) { mclr(mem, mem_size); }

	if(mem && DBG_MEM_ALLOCS_COUNT < (ssize)ARRLEN(DBG_MEM_ALLOCS)) {
		DBG_MEM_ALLOCS[DBG_MEM_ALLOCS_COUNT++] = (struct alloc_meta){
			.file = file,
			.line = line,
			.tag  = tag,
			.size = mem_size,
			.ptr  = mem,
		};
	}

	return mem;
}
#else
static inline void *
alloc_size(struct alloc alloc, ssize mem_size, ssize align, b32 clr)
{
	void *mem = alloc.allocf(alloc.ctx, mem_size, align);
	if(clr) { mclr(mem, mem_size); };
	return mem;
}
#endif

// every object of type struct mkilobyte
// will be aligned to 4-byte boundary
typedef struct mkilobyte {
	alignas(4) char byte[0x400];
} mkilobyte;

typedef struct mmegabyte {
	alignas(4) char byte[0x100000];
} mmegabyte;

typedef struct mspan {
	void *p;
	usize size;
} mspan;

// returns a pointer aligned to the next word address
void *align_up_ptr(void *p);
// returns a pointer aligned to the prev word address
void *align_down_ptr(void *p);
// rounds up to the next multiple of word size
usize align_up_size_t(usize p);
// rounds up to the prev multiple of word size
usize align_down_size_t(usize p);
// places the span beginning at the next word address; size gets adjusted
mspan mspan_align(mspan m);
