#include "adpcm.h"
#include "mathfunc.h"
#include "sys-intrin.h"
#include "sys-utils.h"

static inline i32 adpcm_advance_step(struct adpcm *adpcm);
static inline i32 adpcm_step(u8 step, i16 *history, i16 *step_size);

void
adpcm_reset_to_start(struct adpcm *adpcm)
{
	adpcm->pos      = 0;
	adpcm->data_pos = 0;
	// adpcm->step_size   = 127;
	// adpcm->hist        = 0;
	// adpcm->nibble      = 0;
	// adpcm->curr_sample = 0;
	// adpcm->curr_byte   = 0;
}

void
adpcm_playback(struct adpcm *adpcm, i16 *lb, i16 *rb, i32 len)
{
	i16 *l = lb;
	for(i32 i = 0; i < len; i++, l++) {
		i32 v   = adpcm_advance_step(adpcm);
		i16 res = ssat16((i32)*l + v);
		*l      = v;
	}
	int x = 0;
}

static i32
adpcm_advance_step(struct adpcm *adpcm)
{
	assert(adpcm->pos + 1 < adpcm->len);
	adpcm->pos++;
	if(!adpcm->nibble) {
		adpcm->curr_byte = adpcm->data[adpcm->data_pos++];
	}
	u32 b = (adpcm->curr_byte << adpcm->nibble) >> 4;
	adpcm->nibble ^= 4;
	i16 res = adpcm_step(b, &adpcm->history, &adpcm->step_size);
	return res;
}

// an ADPCM function to advance the sample decoding
// github.com/superctr/adpcm/blob/master/ymb_codec.c
static i32
adpcm_step(u8 step, i16 *history, i16 *step_size)
{
	static const u8 t_step[8] = {57, 57, 57, 57, 77, 102, 128, 153};

	i32 sign    = step & 8;
	i32 delta   = step & 7;
	i32 diff    = ((1 + (delta << 1)) * *step_size) >> 3;
	i32 new_val = *history;
	i32 nstep   = (t_step[delta] * *step_size) >> 6;

	if(sign > 0) {
		new_val -= diff;
	} else {
		new_val += diff;
	}

	// i32 v = ssat16((i32)*history + (sign ? -diff : +diff));
	// i32 nstep   = (t_step[delta] * *step_size) >> 6;
	// *step_size = clamp_i32((t_step[delta] * *step_size) >> 6, 127, 24576);
	*step_size = clamp_i32(nstep, 127, 24576);
	*history = new_val = clamp_i32(new_val, -32768, 32767);
	return new_val * 0.5;
}

void
adpcm_encode(i16 *buffer, u8 *out_buffer, usize len)
{
	int16_t step_size  = 127;
	int16_t history    = 0;
	uint8_t buf_sample = 0, nibble = 0;
	unsigned int adpcm_sample;

	for(usize i = 0; i < len; i++) {
		// we remove a few bits of accuracy to reduce some noise.
		int step     = ((*buffer++) & -8) - history;
		adpcm_sample = (ABS(step) << 16) / (step_size << 14);
		adpcm_sample = MIN(adpcm_sample, 7);
		if(step < 0)
			adpcm_sample |= 8;
		if(nibble)
			*out_buffer++ = buf_sample | (adpcm_sample & 15);
		else
			buf_sample = (adpcm_sample & 15) << 4;
		nibble ^= 1;
		adpcm_step(adpcm_sample, &history, &step_size);
	}
}

void
adpcm_decode(i16 *buffer, u8 *out_buffer, usize len)
{

	int16_t step_size = 127;
	int16_t history   = 0;
	uint8_t nibble    = 0;

	for(usize i = 0; i < len; i++) {
		int8_t step = (*(int8_t *)buffer) << nibble;
		step >>= 4;
		if(nibble)
			buffer++;
		nibble ^= 4;
		*out_buffer++ = adpcm_step(step, &history, &step_size);
	}
}
