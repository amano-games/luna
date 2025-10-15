#pragma once

#include "base/mem.h"
#include "base/types.h"

#define SND_FILE_EXT           "snd"
#define WAVE_FORMAT_PCM        0x1
#define WAVE_FORMAT_IMA_ADPCM  0x11
#define WAVE_FORMAT_EXTENSIBLE 0xfffe

struct wav_riff {
	u32 form_type;
};

struct riff_chunk {
	u32 id;
	u32 size;
};

struct wav_format {
	u16 format;
	u16 channel_count;
	u32 sample_rate;
	u32 byte_rate;
	u16 block_size;
	u16 bits_per_sample;
	u16 cb_size;
	union {
		u16 valid_bits_per_sample;
		u16 samples_per_block;
		u16 reserved;
	} samples;
	i32 channel_mask;
	u16 sub_format;
	char guid[14];
};

struct wav {
	void *sample_data;

	i32 sample_data_size;
	i32 sample_count;
	i32 sample_rate;
	i32 channel_count;
	i32 sample_format;
	i32 block_size;
	i32 samples_per_block;
	i32 bits_per_sample;
};

b32 wav_handle(str8 in_path, str8 out_path, struct alloc scratch);
