#pragma once

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdalign.h>
#include <stddef.h>
#include <inttypes.h>

#define STB_SPRINTF_IMPLEMENTATION
#define STB_SPRINTF_STATIC
#define STB_SPRINTF_NOUNALIGNED
#define STB_SPRINTF_DECORATE(name) sys_##name
#include "stb_sprintf.h"

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef float f32;
typedef double f64;

typedef i8 b8;
typedef i16 b16;
typedef i32 b32;

typedef uintptr_t uptr;
typedef intptr_t iptr;

typedef size_t usize;
typedef ptrdiff_t ssize;

#if defined(TARGET_PLAYDATE)
#define ASM __asm volatile
#endif

#define I64_MAX INT64_MAX
#define I64_MIN INT64_MIN
#define U64_MAX UINT64_MAX
#define U64_MIN 0
#define I32_MAX INT32_MAX
#define I32_MIN INT32_MIN
#define U32_MAX UINT32_MAX
#define U32_MIN 0
#define I16_MAX INT16_MAX
#define I16_MIN INT16_MIN
#define U16_MAX UINT16_MAX
#define U16_MIN 0
#define I8_MAX  INT8_MAX
#define I8_MIN  INT8_MIN
#define U8_MAX  UINT8_MAX
#define U8_MIN  0
#define F32_MAX FLT_MAX
#define F32_MIN FLT_MIN

#define mset              memset
#define mcpy              memcpy
#define mmov              memmove
#define mclr(DST, SIZE)   mset(DST, 0, SIZE)
#define mclr_struct(s)    mclr((s), sizeof(*(s)))
#define mclr_array(a)     mclr((a), sizeof(a))
#define mcpy_struct(d, s) mcpy((d), (s), sizeof(*(d)))
#define mcpy_array(d, s)  mcpy((d), (s), sizeof(d))

typedef struct v2_i32 {
	i32 x, y;
} v2_i32;

typedef struct rec_i32 {
	i32 x, y, w, h;
} rec_i32;

typedef struct v2 {
	f32 x, y;
} v2;

typedef struct v3 {
	f32 x, y, z;
} v3;

typedef struct v4 {
	f32 x, y, z, w;
} v4;

// 2d rotation composed of cos/sin pair for a single angle
// We use two floats as a small optimization to avoid computing sin/cos unnecessarily
typedef struct rot2 {
	f32 c;
	f32 s;
} rot2;

typedef struct {
	f32 m00;
	f32 m01;
	f32 m10;
	f32 m11;
} mat22;

typedef struct str8 {
	u8 *str;
	u64 size;
} str8;

union rng_u64 {
	struct {
		u64 min;
		u64 max;
	};
	u64 v[2];
};

static inline i32
i16_sat(i32 x)
{
	if(x < I16_MIN) return I16_MIN;
	if(x > I16_MAX) return I16_MAX;
	return x;
}
