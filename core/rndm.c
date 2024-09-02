#include "rndm.h"

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
