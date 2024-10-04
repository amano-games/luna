#include "mathfunc.h"
#include "sys-types.h"

i16
sin_wave(f32 time, f32 freq, f32 amp)
{
	static uint32_t count = 0;
	i16 res               = 0;
	f32 tpc               = 44100 / freq; // ticks_per_cycle
	f32 cycles            = count / tpc;
	f32 rad               = PI2_FLOAT * cycles;
	i16 amplitude         = INT16_MAX * amp;
	res                   = amplitude * sin_f32(rad);
	count++;

	return res;
}

i16
square_wave(void)
{
	static uint32_t count = 0;
	i16 half              = INT16_MAX / 2;
	i16 res               = (count++ & (1 << 3)) ? half : -half;
	return res;
}
