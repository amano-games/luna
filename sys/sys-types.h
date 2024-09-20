#pragma once

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdalign.h>
#include <stddef.h>

#define STB_SPRINTF_IMPLEMENTATION
#define STB_SPRINTF_STATIC
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

typedef i8 bool8;
typedef i16 bool16;
typedef i32 bool32;

typedef uintptr_t uptr;
typedef intptr_t iptr;

typedef size_t usize;

typedef struct v2_i32 {
	i32 x, y;
} v2_i32;

typedef struct rec_i32 {
	i32 x, y, w, h;
} rec_i32;

typedef struct v2 {
	f32 x, y;
} v2;

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
