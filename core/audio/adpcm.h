#pragma once

#include "sys-types.h"

struct adpcm {
	u8 *data;     // pointer to sample data (2x 4 bit samples per byte)
	u32 len;      // number of samples
	u32 data_pos; // index in data array
	u32 pos;      // current sample index in original buffer
	i16 history;
	u8 curr_byte;
	i16 step_size;
	u8 nibble;
	// u32 step;
	i16 vol_q8; // playback volume in Q8
};

void adpcm_reset_to_start(struct adpcm *adpcm);
void adpcm_playback(struct adpcm *adpcm, i16 *lb, i16 *rb, i32 len);

void adpcm_encode(i16 *buffer, u8 *out_buffer, usize len);
void adpcm_decode(i16 *buffer, u8 *out_buffer, usize len);
