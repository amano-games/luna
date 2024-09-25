#pragma once

// https://nullprogram.com/blog/2022/08/08/

#include "mem.h"
#include "str.h"
#include "sys-types.h"
#include "sys-assert.h"

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
	u32 mask = ((u64)1 << exp) - 1;
	u32 step = (hash >> (64 - exp)) | 1;
	return (idx + step) & mask;
}

struct ht {
	i32 len;
	int exp;
	char **ht;
};

char *
ht_intern(struct ht *t, char *key)
{
	uint64_t h = hash_string(str8_cstr(key));
	for(int32_t i = h;;) {
		i = ht_lookup(h, t->exp, i);
		if(!t->ht[i]) {
			// empty, insert here
			if((uint32_t)t->len + 1 == (uint32_t)1 << t->exp) {
				return 0; // out of memory
			}
			t->len++;
			t->ht[i] = key;
			return key;
		} else if(!strcmp(t->ht[i], key)) {
			// found, return canonical instance
			return t->ht[i];
		}
	}
}

static struct ht
ht_new(int exp, struct alloc alloc)
{
	struct ht ht = {0, exp, 0};

	assert(exp >= 0);
	if(exp >= 32) {
		return ht; // request too large
	}

	ht.ht = alloc.allocf(alloc.ctx, ((size_t)1 << exp) * sizeof(*ht.ht));

	return ht;
}
