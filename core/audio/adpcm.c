#include "adpcm.h"
#include "mathfunc.h"
#include "sys-intrin.h"
#include "sys-utils.h"

static inline i32 adpcm_advance_step(struct adpcm *adpcm);
static inline i32 adpcm_step(u8 step, i16 *history, i16 *step_size);
static i32 adpcm_step(u8 step, i16 *history, i16 *step_size);
static i32 adpcm_advance_to(struct adpcm *adpcm, u32 pos);
static i32 adpcm_advance_step(struct adpcm *adpcm);

void
adpcm_reset_to_start(struct adpcm *adpcm)
{
	adpcm->step_size   = 127;
	adpcm->history     = 0;
	adpcm->nibble      = 0;
	adpcm->pos         = 0;
	adpcm->pos_pitched = 0;
	adpcm->curr_sample = 0;
	adpcm->data_pos    = 0;
	adpcm->curr_byte   = 0;
}

void
adpcm_set_pitch(struct adpcm *adpcm, i32 pitch_q8)
{
	if(pitch_q8 <= 1) return;
	adpcm->pitch_q8    = pitch_q8;
	adpcm->ipitch_q8   = (256 << 8) / adpcm->pitch_q8;
	adpcm->len_pitched = (adpcm->len * adpcm->pitch_q8) >> 8;

	// highest position in pitched len shall not be out of bounds
	assert((((adpcm->len_pitched - 1) * adpcm->ipitch_q8) >> 8) < adpcm->len);
}

void
adpcm_playback_nonpitch_silent(struct adpcm *adpcm, i32 len)
{
	adpcm->pos_pitched += len;
	adpcm_advance_to(adpcm, adpcm->pos_pitched);
}

void
adpcm_playback(struct adpcm *adpcm, i16 *lb, i16 *rb, i32 len)
{
	i16 *l = lb;
	for(i32 i = 0; i < len; i++, l++) {
		u32 p = (++adpcm->pos_pitched * adpcm->ipitch_q8) >> 8;
		i32 v = (adpcm_advance_to(adpcm, p) * adpcm->vol_q8) >> 8;
		*l    = ssat16((i32)*l + v);
	}
}

void
adpcm_playback_nonpitch(struct adpcm *adpcm, i16 *lb, i16 *rb, i32 len)
{
	i16 *l = lb;
	adpcm->pos_pitched += len;
	for(i32 i = 0; i < len; i++, l++) {
		i32 v = (adpcm_advance_step(adpcm) * adpcm->vol_q8) >> 8;
		*l    = ssat16((i32)*l + v);
	}
}

static i32
adpcm_advance_to(struct adpcm *adpcm, u32 pos)
{
	assert(pos < adpcm->len);
	assert(adpcm->pos <= pos);
	while(adpcm->pos < pos) { // can't savely skip any samples with ADPCM
		adpcm_advance_step(adpcm);
	}
	return adpcm->curr_sample;
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
	adpcm->curr_sample = adpcm_step(b, &adpcm->history, &adpcm->step_size);
	return adpcm->curr_sample;
}

// an ADPCM function to advance the sample decoding
// github.com/superctr/adpcm/blob/master/ymb_codec.c
static i32
adpcm_step(u8 step, i16 *history, i16 *step_size)
{
	static const u8 t_step[8] = {57, 57, 57, 57, 77, 102, 128, 153};

	i32 s      = step & 7;
	i32 d      = ((1 + (s << 1)) * *step_size) >> 3;
	i32 v      = ssat16((i32)*history + ((step & 8) ? -d : +d));
	*step_size = clamp_i32((t_step[s] * *step_size) >> 6, 127, 24576);
	*history   = v;
	return v;
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
		int step     = ((buffer[i]) & -8) - history;
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
