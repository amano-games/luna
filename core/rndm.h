#pragma once

#include "sys-types.h"
#define RND_IMPLEMENTATION
#define RND_U32 u32
#define RND_U64 u64
#include "rnd.h"

struct rndm {
	struct rnd_pcg_t pcg;
};

struct rndm_weighted_choice {
	i32 key;
	u32 value;
};

void rndm_seed(struct rndm *rndm, u32 seed);
i32 rndm_next_i32(struct rndm *rndm);
f32 rndm_next_f32(struct rndm *rndm);
i32 rndm_range_i32(struct rndm *rndm, i32 min, i32 max);
f32 rndm_range_f32(struct rndm *rndm, f32 min, f32 max);
v2 rndm_point_out_rec(struct rndm *rndm, i32 x, i32 y, i32 w, i32 h);
v2 rndm_point_in_rec(struct rndm *rndm, i32 x, i32 y, i32 w, i32 h);
v2 rndm_point_out_cir(struct rndm *rndm, i32 x, i32 y, i32 r);
i32 rndm_weighted_choice_i32(struct rndm *rndm, struct rndm_weighted_choice *choices, usize count);
void rndm_shuffle_arr_f32(struct rndm *rndm, f32 *arr, usize count);
