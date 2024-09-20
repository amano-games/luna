#pragma once

// https://nullprogram.com/blog/2022/08/08/

#include "sys-types.h"
#include "sys-assert.h"

#define HT_EXP 15
#define HT_INIT \
	{ \
		{0}, 0 \
	}

struct ht {
	char *ht[1 << HT_EXP];
	i32 len;
};

static inline i64
hash_x_y(i32 x, i32 y, usize len)
{
	assert(len > 0);
	const i64 h1 = 0x8da6b343; // Large multiplicative constants
	const i64 h2 = 0xd8163841; // here arbitarly chosen primes
	i64 n        = (i64)((h1 * x + h2 * y) % len);

	if(n < 0) n += len;

	return n;
}

// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
// https://nullprogram.com/blog/2022/08/08/
// FNV-1a
u64
hash_string(str8 v)
{
	uint64_t h = 0x100;
	for(u64 i = 0; i < v.size; i++) {
		h ^= v.str[i] & 255;
		h *= 1111111111111111111;
	}
	return h ^ h >> 32;
}

// Compute the next candidate index. Initialize idx to the hash.
i32
ht_lookup(u64 hash, int exp, i32 idx)
{
	u64 mask = ((u64)1 << exp) - 1;
	u64 step = (hash >> (64 - exp)) | 1;
	return (idx + step) & mask;
}
