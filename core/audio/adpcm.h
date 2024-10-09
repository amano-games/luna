#pragma once

#include "sys-types.h"

struct adpcm {
	u8 *data;        // pointer to sample data (2x 4 bit samples per byte)
	u32 len;         // number of samples
	u32 len_pitched; // length of pitched buffer
	u32 data_pos;    // index in data array
	u32 pos;         // current sample index in original buffer
	u32 pos_pitched; // current sample index in pitched buffer
	i16 history;
	u16 pitch_q8;
	u16 ipitch_q8;
	i16 step_size;
	u8 nibble;
	u8 curr_byte;    // current byte value of the sample
	i16 curr_sample; // current decoded i16 sample value
	i16 vol_q8;      // playback volume in Q8
};

void adpcm_reset_to_start(struct adpcm *adpcm);
void adpcm_set_pitch(struct adpcm *adpcm, i32 pitch_q8);
void adpcm_playback(struct adpcm *adpcm, i16 *lb, i16 *rb, i32 len);
void adpcm_playback_nonpitch_silent(struct adpcm *adpcm, i32 len);
void adpcm_playback_nonpitch(struct adpcm *adpcm, i16 *lb, i16 *rb, i32 len);

void adpcm_encode(i16 *buffer, u8 *out_buffer, usize len);
void adpcm_decode(i16 *buffer, u8 *out_buffer, usize len);
