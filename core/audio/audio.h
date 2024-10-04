#pragma once

#include "audio/adpcm.h"
#include "lib/mem.h"
#include "sys-types.h"

#define LEN_MUS_NAME   24
#define NUM_SNDCHANNEL 12
// #define NUM_AUD_CMD_QUEUE 128

enum {
	AUD_MUSCHANNEL_0_LAYER_0,
	AUD_MUSCHANNEL_0_LAYER_1,
	AUD_MUSCHANNEL_0_LAYER_2,
	AUD_MUSCHANNEL_0_LAYER_3,
	AUD_MUSCHANNEL_1,
	AUD_MUSCHANNEL_2,
	//
	NUM_MUSCHANNEL
};

struct sound {
	u8 *buf;
	u32 len;
};

struct mus_channel {
	void *stream;
	u32 total_bytes_file;
	struct adpcm adpcm;

	u8 chunk[256];
	char mus_name[LEN_MUS_NAME];
	i32 trg_vol_q8;
	bool32 looping;
};

struct sfx_channel {
	u32 sfx_id;
	struct adpcm adpcm;
};

struct audio {
	struct mus_channel mus_channel[NUM_MUSCHANNEL];
	struct sfx_channel sfx_channel[NUM_SNDCHANNEL];
	// u32 i_cmd_w_tmp; // write index, copied to i_cmd_w on commit
	// u32 i_cmd_w;     // visible to audio thread/context
	// u32 i_cmd_r;
	// aud_cmd_s cmds[NUM_AUD_CMD_QUEUE];
	// u32 snd_iID; // unique snd instance ID counter
	// bool32 snd_playing_disabled;
	// i32 lowpass;
	// i32 lowpass_acc;
};

static struct audio AUDIO;

void audio_do(i16 *lbuf, i16 *rbuf, i32 len);
void audio_mus_play(const str8 path);

struct sound audio_load(const str8 path, struct alloc alloc);
