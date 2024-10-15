#include "rndm.h"
#include "sys-log.h"
#include "v2.h"

struct rndm RNDM;

void
rndm_seed(u32 seed)
{
	rnd_pcg_seed(&RNDM.pcg, seed);
}

i32
rndm_next_i32(void)
{
	return rnd_pcg_next(&RNDM.pcg);
}

f32
rndm_next_f32(void)
{
	return rnd_pcg_nextf(&RNDM.pcg);
}

i32
rndm_range_i32(i32 min, i32 max)
{
	return rnd_pcg_range(&RNDM.pcg, min, max);
}

f32
rndm_range_f32(f32 min, f32 max)
{
	f32 t = rndm_next_f32();
	return min + t * (max - min);
}

v2
rndm_point_out_rec(i32 x, i32 y, i32 w, i32 h)
{
	f32 p  = rndm_range_i32(0, w + w + h + h);
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
rndm_point_out_cir(i32 x, i32 y, i32 r)
{
	f32 theta = rndm_range_f32(0.0f, 6.0f);
	v2 res    = {cos_f32(theta) * r, sin_f32(theta) * r};
	res       = v2_add((v2){x, y}, res);
	return res;
}

i32
rndm_weighted_choice_i32(struct rndm_weighted_choice *choices, usize count)
{
	i32 sum = 0;

	for(usize i = 0; i < count; i++) {
		struct rndm_weighted_choice choice = choices[i];
		sum += choice.value;
	}
	assert(sum != 0);
	u32 rnd = rndm_next_f32() * sum;
	for(usize i = 0; i < count; i++) {
		if(rnd < choices[i].value) return choices[i].key;
		rnd = rnd - choices[i].value;
	}

	BAD_PATH;
}
