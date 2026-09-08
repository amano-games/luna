#pragma once

#include "base/utils.h"
#include "mem.h"
#include "dbg.h"
#include "base/types.h"

// https://ruby0x1.github.io/machinery_blog_archive/post/minimalist-container-library-in-c-part-1/index.html

struct arr_header {
	ssize len;
	ssize cap;
};

#define arr_new(alloc, ptr, count)     (__typeof__(ptr))arr_ini_internal((alloc), sizeof(*(ptr)), alignof(__typeof__(*ptr)), (count), false)
#define arr_new_clr(alloc, ptr, count) (__typeof__(ptr))arr_ini_internal((alloc), sizeof(*(ptr)), alignof(__typeof__(*ptr)), (count), true)
#define arr_header(a)                  ((a) ? (struct arr_header *)((char *)(a) - sizeof(struct arr_header)) : NULL)
#define arr_pop(a)                     ((a) ? (--arr_header(a)->len, (a)[arr_len(a)]) : (a)[0])
#define arr_len(a)                     ((a) ? arr_header(a)->len : 0)
#define arr_cap(a)                     ((a) ? arr_header(a)->cap : 0)
#define arr_full(a)                    ((a) ? arr_len(a) == arr_cap(a) : true)
#define arr_reset(a)                   ((a) ? arr_header(a)->len = 0 : 0)
#define arr_push(a, item) \
	arr_full(a) ? (a) = arr_grow(a, sizeof(*a)) : 0, (a)[arr_header(a)->len++] = item
#define arr_clr(a) \
	do { \
		if(a) { \
			arr_header(a)->len = 0; \
			mclr(a, sizeof(*(a)) * arr_cap(a)); \
		} \
	} while(0)

static inline void *
arr_ini_internal(struct alloc alloc, ssize elem_size, ssize align, ssize count, b32 clear)
{
	dbg_assert(align > 0);
	dbg_assert(IS_POW2(align));
	// stb-style: payload starts immediately after header
	// only valid if sizeof(header) is already a multiple of the element align.
	dbg_assert(((ssize)sizeof(struct arr_header) & (align - 1)) == 0);

	ssize header_align        = (ssize)alignof(struct arr_header);
	ssize block_align         = MAX(align, header_align);
	ssize new_size            = sizeof(struct arr_header) + count * elem_size;
	struct arr_header *header = alloc_size_aligned(alloc, new_size, block_align, clear);
	dbg_check_mem(header, "arr");

	header->len = 0;
	header->cap = count;

	char *res = (char *)header + sizeof(struct arr_header);

	return res;

error:
	return NULL;
}

static inline void *
arr_grow(void *a, usize size)
{
	// TODO: arr_ini if it's null else bat path
	dbg_sentinel("arr");
error:
	return NULL;
}
