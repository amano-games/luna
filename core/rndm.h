#pragma once

#include "sys-types.h"
#define RND_IMPLEMENTATION
#define RND_U32 u32
#define RND_U64 u64
#include "rnd.h"

struct rndm {
	struct rnd_pcg_t pcg;
};

void rndm_seed(u32 seed);
i32 rndm_next_i32(void);
f32 rndm_next_f32(void);
i32 rndm_range_i32(i32 min, i32 max);
f32 rndm_range_f32(f32 min, f32 max);
