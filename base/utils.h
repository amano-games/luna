#pragma once

#include "base/dbg.h"
#include "base/types.h"
#define MKILOBYTE(value) ((value) * 1024LL)
#define MMEGABYTE(value) (MKILOBYTE(value) * 1024LL)
#define MGIGABYTE(value) (MMEGABYTE(value) * 1024LL)
#define MTERABYTE(value) (MGIGABYTE(value) * 1024LL)

#define MILLION_U32 UINT32_C(1000000)
#define BILLION_U32 UINT32_C(1000000000)
#define MILLION_U64 UINT64_C(1000000)
#define BILLION_U64 UINT64_C(1000000000)
#define MILLION_F32 1e6f
#define BILLION_F32 1e9f
#define MILLION_F64 1e6
#define BILLION_F64 1e9

static const u32 BITMASK_1  = 0x00000001;
static const u32 BITMASK_2  = 0x00000003;
static const u32 BITMASK_3  = 0x00000007;
static const u32 BITMASK_4  = 0x0000000f;
static const u32 BITMASK_5  = 0x0000001f;
static const u32 BITMASK_6  = 0x0000003f;
static const u32 BITMASK_7  = 0x0000007f;
static const u32 BITMASK_8  = 0x000000ff;
static const u32 BITMASK_9  = 0x000001ff;
static const u32 BITMASK_10 = 0x000003ff;
static const u32 BITMASK_11 = 0x000007ff;
static const u32 BITMASK_12 = 0x00000fff;
static const u32 BITMASK_13 = 0x00001fff;
static const u32 BITMASK_14 = 0x00003fff;
static const u32 BITMASK_15 = 0x00007fff;
static const u32 BITMASK_16 = 0x0000ffff;
static const u32 BITMASK_17 = 0x0001ffff;
static const u32 BITMASK_18 = 0x0003ffff;
static const u32 BITMASK_19 = 0x0007ffff;
static const u32 BITMASK_20 = 0x000fffff;
static const u32 BITMASK_21 = 0x001fffff;
static const u32 BITMASK_22 = 0x003fffff;
static const u32 BITMASK_23 = 0x007fffff;
static const u32 BITMASK_24 = 0x00ffffff;
static const u32 BITMASK_25 = 0x01ffffff;
static const u32 BITMASK_26 = 0x03ffffff;
static const u32 BITMASK_27 = 0x07ffffff;
static const u32 BITMASK_28 = 0x0fffffff;
static const u32 BITMASK_29 = 0x1fffffff;
static const u32 BITMASK_30 = 0x3fffffff;
static const u32 BITMASK_31 = 0x7fffffff;
static const u32 BITMASK_32 = 0xffffffff;

static const u32 BIT_1  = (1 << 0);
static const u32 BIT_2  = (1 << 1);
static const u32 BIT_3  = (1 << 2);
static const u32 BIT_4  = (1 << 3);
static const u32 BIT_5  = (1 << 4);
static const u32 BIT_6  = (1 << 5);
static const u32 BIT_7  = (1 << 6);
static const u32 BIT_8  = (1 << 7);
static const u32 BIT_9  = (1 << 8);
static const u32 BIT_10 = (1 << 9);
static const u32 BIT_11 = (1 << 10);
static const u32 BIT_12 = (1 << 11);
static const u32 BIT_13 = (1 << 12);
static const u32 BIT_14 = (1 << 13);
static const u32 BIT_15 = (1 << 14);
static const u32 BIT_16 = (1 << 15);
static const u32 BIT_17 = (1 << 16);
static const u32 BIT_18 = (1 << 17);
static const u32 BIT_19 = (1 << 18);
static const u32 BIT_20 = (1 << 19);
static const u32 BIT_21 = (1 << 20);
static const u32 BIT_22 = (1 << 21);
static const u32 BIT_23 = (1 << 22);
static const u32 BIT_24 = (1 << 23);
static const u32 BIT_25 = (1 << 24);
static const u32 BIT_26 = (1 << 25);
static const u32 BIT_27 = (1 << 26);
static const u32 BIT_28 = (1 << 27);
static const u32 BIT_29 = (1 << 28);
static const u32 BIT_30 = (1 << 29);
static const u32 BIT_31 = (1 << 30);
static const u32 BIT_32 = (1 << 31);

#define LUNA_CONCAT_(a, b) a##b
#define LUNA_CONCAT(a, b)  LUNA_CONCAT_(a, b)

#define LUNA_STRINGIFY_(x) #x
#define LUNA_STRINGIFY(x)  LUNA_STRINGIFY_(x)

#undef MAX
#undef MIN
#define POW2(X)          ((X) * (X))
#define ARRLEN(A)        (sizeof(A) / sizeof(A[0]))
#define MAX(A, B)        ((A) >= (B) ? (A) : (B))
#define MIN(A, B)        ((A) <= (B) ? (A) : (B))
#define ABS(A)           ((A) >= 0 ? (A) : -(A))
#define SGN(A)           ((0 < (A)) - (0 > (A)))
#define CLAMP(X, LO, HI) ((X) > (HI) ? (HI) : ((X) < (LO) ? (LO) : (X)))
#define SWAP(T, a, b) \
	do { \
		T tmp_ = a; \
		a      = b; \
		b      = tmp_; \
	} while(0)

static inline i32
i16_sat(i32 x)
{
	if(x < I16_MIN) return I16_MIN;
	if(x > I16_MAX) return I16_MAX;
	return x;
}

static inline u16
u16_cast_safe(u32 x)
{
	dbg_assert(x <= U16_MAX);
	u16 res = (u16)x;
	return res;
}

static inline u32
u32_cast_safe(u64 x)
{
	dbg_assert(x <= U32_MAX);
	u32 res = (u32)x;
	return res;
}

static inline i32
i32_cast_safe(i64 x)
{
	dbg_assert(x <= I32_MAX);
	i32 res = (i32)x;
	return res;
}

#define ARR_PUSH(xs, x) \
	do { \
		dbg_assert((xs)->count < (xs)->capacity); \
		(xs)->items[(xs)->count++] = (x); \
	} while(0)

// TODO: improve ?
// don't use min_i
#define RING_PUSH(xs, x) \
	do { \
		(xs)->items[(xs)->index] = (x); \
		(xs)->index              = ((xs)->index + 1) % ((xs)->capacity); \
		if((xs)->count < (xs)->capacity) { \
			(xs)->count++; \
		} \
	} while(0)

#define RING_DEF(T) \
	struct T##_ring { \
		usize index; \
		size_t count; \
		size_t capacity; \
		struct T *items; \
	};

#define RING_DEF_SIZE(T, CAPACITY) \
	struct T##_ring { \
		usize index; \
		size_t count; \
		size_t capacity; \
		struct T items[(CAPACITY)]; \
	};
