#pragma once

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
