#pragma once

// https://nullprogram.com/blog/2022/08/08/

#include "mem.h"
#include "sys-types.h"
#include "dbg.h"

static inline i64
hash_x_y(i32 x, i32 y, usize len)
{
	dbg_assert(len > 0);
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

struct ht_entry {
	u64 key;
	u32 value;
};

struct ht_u32 {
	i32 len;
	int exp;
	struct ht_entry *ht;
};

// Compute the next candidate index. Initialize idx to the hash.
i32
ht_lookup(u64 hash, int exp, i32 idx)
{
	u32 mask = ((u64)1 << exp) - 1;
	u32 step = (hash >> (64 - exp)) | 1;
	return (idx + step) & mask;
}

u32
ht_get_u32(struct ht_u32 *t, u64 key)
{
	for(int32_t i = key;;) {
		i = ht_lookup(key, t->exp, i);
		// Empty return 0
		if(t->ht[i].key == 0) {
			return 0;
		} else if(t->ht[i].key == key) {
			return t->ht[i].value;
		}
	}
	return 0;
}

u32
ht_set_u32(struct ht_u32 *t, u64 key, u32 value)
{
	for(int32_t i = key;;) {
		i = ht_lookup(key, t->exp, i);
		// empty, insert here
		if(!t->ht[i].key) {
			if((uint32_t)t->len + 1 == (uint32_t)1 << t->exp) {
				return 0; // out of memory
			}
			t->len++;
			t->ht[i].key   = key;
			t->ht[i].value = value;
			return value;
		}
	}
	return -1;
}

static struct ht_u32
ht_new_u32(int exp, struct alloc alloc)
{
	struct ht_u32 ht = {0, exp, 0};

	dbg_assert(exp >= 0);
	if(exp >= 32) {
		return ht; // request too large
	}

	usize size = ((size_t)1 << exp) * sizeof(*ht.ht);
	ht.ht      = alloc.allocf(alloc.ctx, size);
	mclr(ht.ht, size);
	return ht;
}
