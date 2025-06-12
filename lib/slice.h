#pragma once

#if 0
#include "mem.h"
#include <stddef.h>

// https://nullprogram.com/blog/2023/09/27/
// https://nullprogram.com/blog/2023/10/05/
#define slice_push(s, arena) \
	((s)->len >= (s)->cap \
		? grow(s, sizeof(*(s)->data), arena), \
		(s)->data + (s)->len++ \
		: (s)->data + (s)->len++)

static void
slice_grow(void *slice, ptrdiff_t size, struct alloc alloc)
{
	struct {
		void *data;
		ptrdiff_t len;
		ptrdiff_t cap;
	} replica;
	mcpy(&replica, slice, sizeof(replica));

	replica.cap = replica.cap ? replica.cap : 1;
	void *data  = alloc.allocf(alloc.ctx, 2 * size, replica.cap);
	replica.cap *= 2;
	if(replica.len) {
		memcpy(data, replica.data, size * replica.len);
	}
	replica.data = data;

	memcpy(slice, &replica, sizeof(replica));
}
#endif
