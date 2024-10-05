#pragma once

#include "sys-types.h"

#ifdef TARGET_PLAYDATE
#define bswap32 __builtin_bswap32
static inline u32
brev32(u32 v)
{
	u32 r;
	__asm("rbit %0, %1"
		  : "=r"(r)
		  : "r"(v));
	return r;
}

// static inline i32
// ssat16(i32 x)
// {
// 	u32 r = 0;
// 	u32 i = (u32)x;
// 	ASM("ssat %0, %1, %2"
// 		: "=r"(r)
// 		: "I"(16), "r"(i));
// 	return (i32)r;
// }
#else
static u32
bswap32(u32 i)
{
	return (i >> 24) | ((i << 8) & 0xFF0000U) | (i << 24) | ((i >> 8) & 0x00FF00U);
}

static u32
brev32(u32 x)
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

#endif
static inline i32
ssat16(i32 x)
{
	if(x < I16_MIN) return I16_MIN;
	if(x > I16_MAX) return I16_MAX;
	return x;
}
