#pragma once

#include "base/types.h"
#include <stdalign.h>

#define MKILOBYTE(value) ((value) * 1024LL)
#define MMEGABYTE(value) (MKILOBYTE(value) * 1024LL)
#define MGIGABYTE(value) (MMEGABYTE(value) * 1024LL)
#define MTERABYTE(value) (MGIGABYTE(value) * 1024LL)

#define MEM_ALIGN_BYTE     1
#define MEM_ALIGN_DEFAULT  8  // Arena seeds, file blobs, frame scratch
#define MEM_ALIGN_PD_CACHE 32 // PD Cortex-M7 cache line / SIMD-ish

#define ALIGN_POW2(x, b)      (((x) + (b) - 1) & ~((b) - 1))
#define ALIGN_DOWN_POW2(x, b) ((x) & ~((b) - 1))
#define IS_POW2(x)            ((x) != 0 && ((x) & ((x) - 1)) == 0)

// used for user defined allocations
// alloc(ctx, size) -> ctx: pointer to some memory manager
struct alloc {
	void *(*allocf)(void *ctx, ssize size, ssize align);
	void *ctx;
};

// Alloc / align contract:
// - If you cast the result to T*, allocate with alignof(T), alignas on T, or *_aligned.

#define mem_alloc_size(a, size)     alloc_size_aligned((a), (size), MEM_ALIGN_DEFAULT, false)
#define mem_alloc_size_clr(a, size) alloc_size_aligned((a), (size), MEM_ALIGN_DEFAULT, true)

#define alloc_struct(alloc, ptr)                (__typeof__(ptr))alloc_size_aligned(alloc, sizeof(*(ptr)), alignof(__typeof__(*(ptr))), false)
#define alloc_struct_aligned(alloc, ptr, align) (__typeof__(ptr))alloc_size_aligned(alloc, sizeof(*(ptr)), align, false)
#define alloc_struct_clr(alloc, ptr)            (__typeof__(ptr))alloc_size_aligned(alloc, sizeof(*(ptr)), alignof(__typeof__(*(ptr))), true)

#define alloc_arr(alloc, ptr, count)                (__typeof__(ptr))alloc_size_aligned((alloc), sizeof(*(ptr)) * (count), alignof(__typeof__(*(ptr))), false)
#define alloc_arr_aligned(alloc, ptr, count, align) (__typeof__(ptr))alloc_size_aligned((alloc), sizeof(*(ptr)) * (count), align, false)
#define alloc_arr_clr(alloc, ptr, count)            (__typeof__(ptr))alloc_size_aligned((alloc), sizeof(*(ptr)) * (count), alignof(__typeof__(*(ptr))), true)

static inline void *
alloc_size_aligned(struct alloc alloc, ssize mem_size, ssize align, b32 clr)
{
	void *mem = alloc.allocf(alloc.ctx, mem_size, align);
	if(clr) { mclr(mem, mem_size); };
	return mem;
}
