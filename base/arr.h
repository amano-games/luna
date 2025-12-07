#pragma once

#include "mem.h"
#include "dbg.h"
#include "base/types.h"

// https://ruby0x1.github.io/machinery_blog_archive/post/minimalist-container-library-in-c-part-1/index.html

struct arr_header {
	ssize len;
	ssize cap;
};

#define arr_new(alloc, ptr, count)     (__typeof__(ptr))_arr_ini((alloc), sizeof(*(ptr)), alignof(max_align_t), (count), false)
#define arr_new_clr(alloc, ptr, count) (__typeof__(ptr))_arr_ini((alloc), sizeof(*(ptr)), alignof(max_align_t), (count), true)
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

#define arr_push_packed(ptr, item, alloc) \
	arr_full(ptr) ? (ptr) = arr_grow_packed(ptr, arr_len(ptr) + 1, sizeof(*(ptr)), alignof(*(ptr)), alloc) : 0, (ptr)[arr_header(ptr)->len++] = item

void *
_arr_ini(struct alloc alloc, ssize elem_size, ssize align, ssize count, b32 clear)
{
	// TODO: use align instead of max align_t
	usize new_size            = sizeof(struct arr_header) + count * elem_size;
	struct arr_header *header = alloc.allocf(alloc.ctx, new_size, alignof(max_align_t));
	dbg_check_mem(header, "arr");
	header->len = 0;
	header->cap = count;
	char *res   = (char *)header + sizeof(struct arr_header);
	if(clear) {
		mclr(res, count * elem_size);
	}
	return res;

error:
	return NULL;
}

void *
arr_grow(void *a, usize size)
{
	// TODO: arr_ini if it's null else bat path
	dbg_sentinel("arr");
error:
	return NULL;
}

void *
arr_grow_packed(void *a, ssize new_len, ssize elem_size, ssize elem_align, struct alloc alloc)
{
	struct arr_header *header = a ? arr_header(a) : arr_header(_arr_ini(alloc, elem_size, elem_align, new_len, false));
	usize new_cap             = new_len;

	ssize len   = arr_len(a);
	ssize count = new_len - len;

	void *res = alloc.allocf(alloc.ctx, count * elem_size, 4);
	// TODO: Check if packed

	header->cap = new_cap;
	void *arr   = (char *)header + sizeof(struct arr_header);
	return arr;
}
