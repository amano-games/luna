#pragma once

#include "base/context-cracking.h"
#include "base/types.h"

#if COMPILER_MSVC
#include <intrin.h>
#include <stdlib.h>
#endif

// byteswap

#if COMPILER_CLANG || COMPILER_GCC
#define bswap_u16 __builtin_bswap16
#define bswap_u32 __builtin_bswap32
#define bswap_u64 __builtin_bswap64
#elif COMPILER_MSVC
#define bswap_u16 _byteswap_ushort
#define bswap_u32 _byteswap_ulong
#define bswap_u64 _byteswap_uint64
#endif

// bit reverse
#if COMPILER_CLANG
#define brev_u32 __builtin_bitreverse32
#elif COMPILER_GCC && ARCH_ARM32 && __ARM_ARCH >= 7
#define brev_u32 __builtin_arm_rbit
#elif COMPILER_GCC && ARCH_ARM64
static inline u32
brev_u32(u32 v)
{
	u32 r;
	__asm volatile("rbit %w0, %w1" : "=r"(r) : "r"(v));
	return r;
}
#else
static inline u32
brev_u32(u32 x)
{
	x = ((x >> 1) & 0x55555555u) | ((x & 0x55555555u) << 1);
	x = ((x >> 2) & 0x33333333u) | ((x & 0x33333333u) << 2);
	x = ((x >> 4) & 0x0f0f0f0fu) | ((x & 0x0f0f0f0fu) << 4);
	x = ((x >> 8) & 0x00ff00ffu) | ((x & 0x00ff00ffu) << 8);
	x = (x >> 16) | (x << 16);
	return x;
}
#endif

// signed saturate to i16
#if (COMPILER_CLANG || COMPILER_GCC) && ARCH_ARM32 && __ARM_ARCH >= 6
#define ssat_i16(x) ((i32)__builtin_arm_ssat((x), 16))
#elif COMPILER_MSVC && ARCH_ARM32
#define ssat_i16(x) ((i32)_arm_ssat((x), 16))
#else
static inline i32
ssat_i16(i32 x)
{
	if(x < I16_MIN) {
		return I16_MIN;
	}
	if(x > I16_MAX) {
		return I16_MAX;
	}
	return x;
}
#endif
