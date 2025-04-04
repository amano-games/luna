#pragma once

#include "mem.h"
#include "sys-types.h"

#define SND_FILE_EXT           "snd"
#define WAVE_FORMAT_PCM        0x1
#define WAVE_FORMAT_IMA_ADPCM  0x11
#define WAVE_FORMAT_EXTENSIBLE 0xfffe

struct riff_chunk_header {
	char chunk_id[4];
	i32 chunk_size;
	char form_type[4];
};

struct chunk_header {
	char chunk_id[4];
	uint32_t chunk_size;
};

struct wave_header {
	u16 format_tag;
	u16 num_channels;
	u32 sample_rate, bytes_per_second;
	u16 block_align, bits_per_sample;
	u16 cb_size;
	union {
		u16 valid_bits_per_sample;
		u16 samples_per_block;
		u16 reserved;
	} samples;
	int32_t channel_mask;
	u16 sub_format;
	char guid[14];
};

struct wav {
	void *data;
};

int handle_wav(str8 in_path, str8 out_path, struct alloc scratch);
