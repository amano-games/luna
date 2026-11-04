#pragma once

// MSI hash table: https://nullprogram.com/blog/2022/08/08/
// Open addressing
// ht_lookup double-hashing probe step
//
// TODO:
// - https://nullprogram.com/blog/2023/06/26/
// - https://nullprogram.com/blog/2023/09/30/
// - https://nullprogram.com/blog/2025/01/19/
// - https://nullprogram.com/blog/2026/05/06/

#include "base/mem.h"
#include "base/types.h"
#include "base/dbg.h"

struct ht_entry_u32 {
	u64 key;
	u32 value;
};

struct ht_u32 {
	i32 len;
	int exp;
	struct ht_entry_u32 *ht;
};

struct ht_entry_u64 {
	u64 key;
	u64 value;
};

struct ht_u64 {
	i32 len;
	int exp;
	struct ht_entry_u64 *ht;
};

// Compute the next candidate index. Initialize idx to the hash.
static inline i32
ht_lookup(u64 hash, int exp, i32 idx)
{
	u32 mask = ((u64)1 << exp) - 1;
	u32 step = (hash >> (64 - exp)) | 1;
	i32 res  = (idx + step) & mask;
	return res;
}

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

static inline u32
ht_u32_get(struct ht_u32 *t, u64 key)
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

static inline u32
ht_u32_insert(struct ht_u32 *t, u64 key, u32 value)
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
		} else if(t->ht[i].key == key) {
			res = t->ht[i].value;
			goto cleanup;
		}
	}
cleanup:
	return res;
}

static inline struct ht_u32
ht_u32_new(int exp, struct alloc alloc)
{
	struct ht_u32 ht = {0, exp, 0};

	dbg_assert(exp >= 0);
	if(exp >= 32) {
		return ht; // request too large
	}

	ssize size = ((size_t)1 << exp) * sizeof(*ht.ht);
	ht.ht      = alloc_size_aligned(alloc, size, alignof(struct ht_entry_u32), true);
	return ht;
}

static inline u64
ht_u64_get(struct ht_u64 *t, u64 key)
{
	u64 res = 0;
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

static inline u64
ht_u64_insert(struct ht_u64 *t, u64 key, u64 value)
{
	u64 res = -1;
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
		} else if(t->ht[i].key == key) {
			res = t->ht[i].value;
			goto cleanup;
		}
	}
cleanup:
	return res;
}

static inline struct ht_u64
ht_u64_new(int exp, struct alloc alloc)
{
	struct ht_u64 ht = {0, exp, 0};

	dbg_assert(exp >= 0);
	if(exp >= 32) {
		return ht; // request too large
	}

	ssize size = ((size_t)1 << exp) * sizeof(*ht.ht);
	ht.ht      = alloc_size_aligned(alloc, size, alignof(struct ht_entry_u64), true);
	return ht;
}

// Open addressing needs a spare slot. Size ~2x so probes stay short.
static i32
ht_exp_from_count(usize count)
{
	usize need = (count + 1) * 2;
	i32 exp    = 1;

	while(((usize)1 << exp) < need) {
		exp++;
		dbg_assert(exp < 31);
	}

	return exp;
}
