#pragma once

#include "base/types.h"

#if defined(TARGET_PLAYDATE)
#define bswap_u32 __builtin_bswap32

static inline u32
brev_u32(u32 v)
{
	u32 r;
	ASM("rbit %0, %1" : "=r"(r) : "r"(v));
	return r;
}

static inline i32
ssat16(i32 x)
{
	u32 r = 0;
	u32 i = (u32)x;
	ASM("ssat %0, %1, %2" : "=r"(r) : "I"(16), "r"(i));
	return (i32)r;
}
#else
static u32
bswap_u32(u32 i)
{
	u32 res = (((i & 0xFF000000) >> 24) |
		((i & 0x00FF0000) >> 8) |
		((i & 0x0000FF00) << 8) |
		((i & 0x000000FF) << 24));
	return res;
}

static u32
brev_u32(u32 x)
{
	u32 r = x;
	int s = 31;
	for(u32 v = x >> 1; v; v >>= 1) {
		r <<= 1;
		r |= v & 1;
		s--;
	}
	return (r << s);
}

static inline i32
ssat16(i32 x)
{
	if(x < I16_MIN) return I16_MIN;
	if(x > I16_MAX) return I16_MAX;
	return x;
}

#endif

static inline u16
bswap_u16(u16 x)
{
	u16 res = (((x & 0xFF00) >> 8) |
		((x & 0x00FF) << 8));
	return res;
}

static inline u64
bswap_u64(u64 x)
{
	// TODO: naive bswap, replace with something
	// that is faster like an intrinsic
	u64 res = (((x & 0xFF00000000000000ULL) >> 56) |
		((x & 0x00FF000000000000ULL) >> 40) |
		((x & 0x0000FF0000000000ULL) >> 24) |
		((x & 0x000000FF00000000ULL) >> 8) |
		((x & 0x00000000FF000000ULL) << 8) |
		((x & 0x0000000000FF0000ULL) << 24) |
		((x & 0x000000000000FF00ULL) << 40) |
		((x & 0x00000000000000FFULL) << 56));
	return res;
}
