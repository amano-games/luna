#pragma once

// https://danielchasehooper.com/posts/typechecked-generic-c-data-structures/

#include "base/types.h"

struct ring_internal {
	u32 idx;
	u32 len;
	u32 cap;
	char *items;
};

#define Ring(type) \
	union { \
		struct ring_internal internal; \
		type *payload; \
	}

void *
_ring_peek(struct ring_internal ring, size_t elem_size)
{
	return ring.items + (ring.idx * elem_size);
}

// #define ring_push(r, value) \
// 	((typeof((sa)->payload))_sa_get(&(sa)->internal, \
// 		index, \
// 		sizeof(*(sa)->payload)))
