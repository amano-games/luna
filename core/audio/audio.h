#pragma once

#include "assets/asset-db.h"
#include "audio/adpcm.h"
#include "lib/mem.h"
#include "sys-types.h"

#define LEN_MUS_NAME      24
#define NUM_SND_CHANNEL   12
#define NUM_AUD_CMD_QUEUE 128

enum mus_channel_id {
	AUD_MUS_CHANNEL_NONE,

	AUD_MUS_CHANNEL_1,
	AUD_MUS_CHANNEL_2,
	AUD_MUS_CHANNEL_3,
	AUD_MUS_CHANNEL_4,

	AUD_MUS_CHANNEL_NUM_COUNT
};

enum {
	AUD_CMD_NONE,

	AUD_CMD_SND_PLAY,
	AUD_CMD_SND_STOP,
	AUD_CMD_SND_MODIFY_VOL,
	AUD_CMD_SND_MODIFY_REPEAT_COUNT,

	AUD_CMD_MUS_PLAY,
	AUD_CMD_MUS_STOP,
	AUD_CMD_MUS_MODIFY_VOL,

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
	u16 repeat_count;
};

struct aud_cmd_snd_modify {
	u32 id;
	u16 vol_q8;
	u16 repeat_count;
};

struct aud_cmd_mus_play {
	struct asset_handle path_handle;
	u8 channel_id;
	u8 vol_q8;
};

struct aud_cmd_mus_modify {
	u8 channel_id;
	u8 vol_q8;
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
		struct aud_cmd_mus_modify mus_modify;

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
	u16 repeat_count;
	u16 count;
};

struct aud {
	struct mus_channel mus_channel[AUD_MUS_CHANNEL_NUM_COUNT];
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
u32 snd_instance_play(struct snd s, f32 vol, f32 pitch, u16 repeat_count); // returns an integer to refer to an active snd instance
bool32 snd_instance_is_playing(u32 snd_id);
void snd_instance_stop(u32 snd_id);
void snd_instance_set_repeat_count(u32 snd_id, u16 repeat_count);
void snd_instance_set_vol(u32 snd_id, f32 vol);

void mus_play(const struct asset_handle handle, enum mus_channel_id channel_id, f32 vol);
void mus_play_by_path(const str8 path, enum mus_channel_id channel_id, f32 vol);
bool32 mus_is_playing(enum mus_channel_id channel_id);
void mus_set_vol(enum mus_channel_id channel_id, f32 vol);
void mus_stop(enum mus_channel_id channel_id);
