#pragma once

// https://nullprogram.com/blog/2022/08/08/

#include "base/mem.h"
#include "base/types.h"
#include "base/dbg.h"
#include "base/trace.h"

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
hash_fnv1a_str8(str8 v)
{
	u64 h = 0x100;
	for(u64 i = 0; i < v.size; i++) {
		h ^= v.str[i] & 255;
		h *= 1111111111111111111;
	}
	return h ^ h >> 32;
}

// MurmurOAAT64
// https://phoboslab.org/log/2024/09/qop
u64
hash_murmuroaat_str8(str8 v)
{
	u64 h = 525201411107845655ull;
	for(u64 i = 0; i < v.size; i++) {
		h ^= v.str[i];
		h *= 0x5bd1e9955bd1e995ull;
		h ^= h >> 47;
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
	i32 res  = (idx + step) & mask;
	return res;
}

u32
ht_get_u32(struct ht_u32 *t, u64 key)
{
	u32 res = 0;
	for(int32_t i = key;;) {
		i = ht_lookup(key, t->exp, i);
		// Empty return 0
		if(t->ht[i].key == 0) {
			goto cleanup;
		} else if(t->ht[i].key == key) {
			res = t->ht[i].value;
			goto cleanup;
		}
	}
cleanup:
	return res;
}

u32
ht_set_u32(struct ht_u32 *t, u64 key, u32 value)
{
	u32 res = -1;
	for(int32_t i = key;;) {
		i = ht_lookup(key, t->exp, i);
		// empty, insert here
		if(!t->ht[i].key) {
			if((uint32_t)t->len + 1 == (uint32_t)1 << t->exp) {
				res = 0; // out of memory
				goto cleanup;
			}
			t->len++;
			t->ht[i].key   = key;
			t->ht[i].value = value;
			res            = value;
			goto cleanup;
		}
	}
cleanup:
	return res;
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
	ht.ht      = alloc.allocf(alloc.ctx, size, 4);
	mclr(ht.ht, size);
	return ht;
}
