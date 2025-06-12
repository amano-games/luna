#pragma once

#include "mem.h"
#include "dbg.h"
#include "sys-types.h"

// https://ruby0x1.github.io/machinery_blog_archive/post/minimalist-container-library-in-c-part-1/index.html

struct arr_header {
	usize len;
	usize cap;
};

#define arr_new(a, c, m)     _arr_ini(sizeof(*(a)), (c), (m))
#define arr_new_clr(a, c, m) arr_ini_clr(sizeof(*(a)), (c), (m))
#define arr_header(a)        ((a) ? (struct arr_header *)((char *)(a) - sizeof(struct arr_header)) : NULL)
#define arr_pop(a)           ((a) ? (--arr_header(a)->len, (a)[arr_len(a)]) : (a)[0])
#define arr_len(a)           ((a) ? arr_header(a)->len : 0)
#define arr_cap(a)           ((a) ? arr_header(a)->cap : 0)
#define arr_full(a)          ((a) ? arr_len(a) == arr_cap(a) : true)
#define arr_reset(a)         ((a) ? arr_header(a)->len = 0 : 0)
#define arr_push(a, item) \
	arr_full(a) ? (a) = arr_grow(a, sizeof(*a)) : 0, (a)[arr_header(a)->len++] = item
#define arr_clr(a) \
	do { \
		if(a) { \
			arr_header(a)->len = 0; \
			mclr(a, sizeof(*(a)) * arr_cap(a)); \
		} \
	} while(0)

#define arr_push_packed(a, item, alloc) \
	arr_full(a) ? (a) = arr_grow_packed(a, arr_len(a) + 1, sizeof(*a), alloc) : 0, (a)[arr_header(a)->len++] = item

void *
_arr_ini(usize elem_size, usize count, struct alloc alloc)
{
	usize new_size            = sizeof(struct arr_header) + count * elem_size;
	struct arr_header *header = alloc.allocf(alloc.ctx, new_size);
	dbg_check_mem(header, "arr");
	header->len = 0;
	header->cap = count;
	char *res   = (char *)header + sizeof(struct arr_header);
	return res;

error:
	return NULL;
}

void *
arr_ini_clr(usize elem_size, usize count, struct alloc alloc)
{
	usize new_size            = sizeof(struct arr_header) + count * elem_size;
	struct arr_header *header = alloc.allocf(alloc.ctx, new_size);
	dbg_check_mem(header, "arr");
	header->len = 0;
	header->cap = count;
	char *res   = (char *)header + sizeof(struct arr_header);
	mclr(res, count * elem_size);
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
arr_grow_packed(void *a, usize new_len, usize elem_size, struct alloc alloc)
{
	struct arr_header *header = a ? arr_header(a) : arr_header(_arr_ini(elem_size, new_len, alloc));
	usize new_cap             = new_len;

	usize len   = arr_len(a);
	usize count = new_len - len;

	void *res = alloc.allocf(alloc.ctx, count * elem_size);
	// TODO: Check if packed

	header->cap = new_cap;
	void *arr   = (char *)header + sizeof(struct arr_header);
	return arr;
}
