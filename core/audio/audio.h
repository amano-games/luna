#pragma once

#include "assets/asset-db.h"
#include "audio/adpcm.h"
#include "lib/mem.h"
#include "sys-types.h"

#define LEN_MUS_NAME      24
#define NUM_SND_CHANNEL   12
#define NUM_AUD_CMD_QUEUE 128

enum {
	AUD_MUS_CHANNEL_0_LAYER_0,
	AUD_MUS_CHANNEL_0_LAYER_1,
	AUD_MUS_CHANNEL_0_LAYER_2,
	AUD_MUS_CHANNEL_0_LAYER_3,
	AUD_MUS_CHANNEL_1,
	AUD_MUS_CHANNEL_2,

	NUM_MUS_CHANNEL
};

enum {
	AUD_CMD_SND_PLAY,
	AUD_CMD_SND_MODIFY,
	AUD_CMD_MUS_PLAY,
	AUD_CMD_LOWPASS,
};

#define AUD_CMD_PRIORITY_MUS_PLAY 1

struct snd {
	u8 *buf;
	u32 len;
};

struct aud_cmd_snd_play {
	struct snd snd;
	u32 id;
	u16 vol_q8;
	u16 pitch_q8;
};

struct aud_cmd_snd_modify {
	u32 id;
	bool16 stop;
	u16 vol_q8;
};

struct aud_cmd_mus_play {
	struct asset_handle path_handle;
	u8 channel_id;
	u8 vol_q8;
	u8 ticks_out;
	u8 ticks_in;
};

struct aud_cmd_lowpass {
	i32 v;
};

struct aud_cmd {
	// alignas(32) // cache line on Cortex M7
	u16 type;
	u16 priority; // for dropping unimportant commands when the queue is full
	union {
		struct aud_cmd_snd_play snd_play;
		struct aud_cmd_snd_modify snd_modify;
		struct aud_cmd_mus_play mus_play;
		struct aud_cmd_lowpass lowpass;
	} c;
};

struct mus_channel {
	void *stream;
	u32 total_bytes_file;
	struct adpcm adpcm;
	u8 chunk[256];
	struct asset_handle path_handle;
	i32 trg_vol_q8;
	bool32 looping;
};

struct sfx_channel {
	u32 snd_id;
	struct adpcm adpcm;
};

struct aud {
	struct mus_channel mus_channel[NUM_MUS_CHANNEL];
	struct sfx_channel sfx_channel[NUM_SND_CHANNEL];
	u32 i_cmd_w_tmp; // write index, copied to i_cmd_w on commit
	u32 i_cmd_w;     // visible to audio thread/context
	u32 i_cmd_r;
	struct aud_cmd cmds[NUM_AUD_CMD_QUEUE];
	u32 snd_id; // unique snd instance ID counter
	bool32 snd_playing_disabled;
	i32 lowpass;
	i32 lowpass_acc;
};

static struct aud AUDIO;

void aud_do(i16 *lbuf, i16 *rbuf, i32 len);
void aud_allow_playing_new_snd(bool32 enabled);
void aud_set_lowpass(i32 lp); // 0 for off, otherwise increasing intensity
void aud_cmd_queue_commit(void);

struct snd snd_load(const str8 path, struct alloc alloc);
u32 snd_instance_play(struct snd s, f32 vol, f32 pitch); // returns an integer to refer to an active snd instance
void snd_instance_stop(u32 snd_id);
void snd_instance_set_vol(u32 snd_id, f32 vol);

void mus_play(const struct asset_handle handle, f32 vol);
void mus_play_by_path(const str8 path, f32 vol);
