#pragma once

#include "base/types.h"

// https://en.wikipedia.org/wiki/Fowler%E2%80%93Noll%E2%80%93Vo_hash_function
// https://nullprogram.com/blog/2022/08/08/
// FNV-1a
static inline u64
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
static inline u64
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
