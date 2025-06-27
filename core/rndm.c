#include "rndm.h"
#include "dbg.h"
#include "sys-log.h"
#include "v2.h"

struct rndm RNDM;

void
rndm_seed(struct rndm *rndm, u32 seed)
{
	if(rndm == NULL) { rndm = &RNDM; }
	rnd_pcg_seed(&rndm->pcg, seed);
}

i32
rndm_next_i32(struct rndm *rndm)
{
	if(rndm == NULL) { rndm = &RNDM; }
	return rnd_pcg_next(&rndm->pcg);
}

f32
rndm_next_f32(struct rndm *rndm)
{
	if(rndm == NULL) { rndm = &RNDM; }
	return rnd_pcg_nextf(&rndm->pcg);
}

i32
rndm_range_i32(struct rndm *rndm, i32 min, i32 max)
{
	if(rndm == NULL) { rndm = &RNDM; }
	return rnd_pcg_range(&rndm->pcg, min, max);
}

f32
rndm_range_f32(struct rndm *rndm, f32 min, f32 max)
{
	f32 t = rndm_next_f32(rndm);
	return min + t * (max - min + EPSILON);
}

v2
rndm_point_out_rec(struct rndm *rndm, i32 x, i32 y, i32 w, i32 h)
{
	f32 p  = rndm_range_i32(rndm, 0, w + w + h + h);
	f32 px = 0;
	f32 py = 0;

	if(p < (w + h)) {
		if(p < w) {
			px = p;
			py = 0;
		} else {
			px = w;
			py = p - w;
		}
	} else {
		p = p - (w + h);
		if(p < w) {
			px = w - p;
			py = h;
		} else {
			px = 0;
			py = h - (p - w);
		}
	}

	return (v2){px + x, py + y};
}

v2
rndm_point_in_rec(struct rndm *rndm, i32 x, i32 y, i32 w, i32 h)
{
	v2 res = {
		.x = rndm_range_f32(rndm, x, x + w),
		.y = rndm_range_f32(rndm, y, y + h),
	};
	return res;
}

v2
rndm_point_out_cir(struct rndm *rndm, i32 x, i32 y, i32 r)
{
	f32 angle = rndm_next_f32(rndm) * TURN_TO_RAD;
	v2 res    = v2_from(angle, r);
	res       = v2_add((v2){x, y}, res);
	return res;
}

i32
rndm_weighted_choice_i32(
	struct rndm *rndm,
	struct rndm_weighted_choice *choices,
	usize count)
{
	i32 sum = 0;

	for(usize i = 0; i < count; i++) {
		struct rndm_weighted_choice choice = choices[i];
		sum += choice.value;
	}
	dbg_assert(sum != 0);
	u32 rnd = rndm_next_f32(rndm) * sum;
	for(usize i = 0; i < count; i++) {
		if(choices[i].value == 0) { continue; }
		if(rnd < choices[i].value) return choices[i].key;
		rnd = rnd - choices[i].value;
	}

	dbg_sentinel("rndm");
error:
	return 0;
}

void
rndm_shuffle_arr_f32(struct rndm *rndm, f32 *arr, usize count)
{
	if(count > 1) {
		for(usize i = 0; i < count - 1; i++) {
			usize j = i + rndm_range_i32(rndm, 0, count - i - 1);
			f32 t   = arr[j];
			arr[j]  = arr[i];
			arr[i]  = t;
		}
	}
}
