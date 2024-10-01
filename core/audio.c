#include "audio.h"
#include "mathfunc.h"
#include "sys-io.h"
#include "sys-log.h"

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

void
audio_do(i16 *lbuf, i16 *rbuf, i32 len)
{
	const f32 two_pi = PI2_FLOAT;
	f32 amplitude    = 1.0f;     // Volume ?
	f32 frequency    = 440.0f;   // Default frequency (A4 - 440 Hz)
	f32 sample_rate  = 44100.0f; // 44.1 kHz sample rate

	for(usize i = 0; i < (usize)len; i++) {
		i16 wave = sin_wave(0, frequency, amplitude);
		lbuf[i]  = wave;
		rbuf[i]  = wave;
	}
}

struct sound
audio_load(const str8 path, struct alloc alloc)
{
	struct sound res = {0};

	void *f = sys_file_open_r(path);
	if(!f) {
		sys_printf("+++ ERR: can't open file for snd %s\n", path.str);
		return res;
	}

	// u32 num_samples = 0;
	// sys_file_r(f, &num_samples, sizeof(u32));
	// u32 bytes = (num_samples + 1) >> 1;
	//
	// void *buf = alloc.allocf(alloc.ctx, bytes);
	// if(!buf) {
	// 	sys_printf("+++ ERR: can't allocate memory for snd %s\n", path.str);
	// 	sys_file_close(f);
	// 	return snd;
	// }
	//
	// sys_file_r(f, buf, bytes);
	// sys_file_close(f);
	//
	// snd.buf = (u8 *)buf;
	// snd.len = num_samples;
	return res;
}
