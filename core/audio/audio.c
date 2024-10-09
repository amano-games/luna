#include "audio.h"
#include "assets/asset-db.h"
#include "assets/assets.h"
#include "audio/adpcm.h"
#include "ht.h"
#include "mathfunc.h"
#include "sys-io.h"
#include "sys-log.h"
#include "sys-types.h"
#include "sys-utils.h"
#include "sys.h"

static void aud_cmds_flush(void);
static void aud_push_cmd(struct aud_cmd aud_cmd);
static inline u32 aud_cmd_next_index(u32 i);

static void mus_channel_playback(struct mus_channel *mc, i16 *lb, i16 *rb, i32 len);
static void mus_channel_playback_part(struct mus_channel *mc, i16 *lb, i16 *rb, i32 len);
static void mus_channel_stop(struct mus_channel *mc);

static void sfx_channel_playback(struct sfx_channel *sc, i16 *lbuf, i16 *rbuf, i32 len);
static void sfx_channel_stop(struct sfx_channel *ch);

void
aud_do(i16 *lbuf, i16 *rbuf, i32 len)
{
	aud_cmds_flush();

	for(usize n = 0; n < ARRLEN(AUDIO.mus_channel); n++) {
		struct mus_channel *ch = &AUDIO.mus_channel[n];
		ch->adpcm.vol_q8       = 64;
		mus_channel_playback(ch, lbuf, rbuf, len);
	}

	for(i32 n = 0; n < NUM_SND_CHANNEL; ++n) {
		struct sfx_channel *sfx_channel = &AUDIO.sfx_channel[n];
		sfx_channel_playback(sfx_channel, lbuf, rbuf, len);
	}

	if(AUDIO.lowpass) {
		i16 *b = lbuf;
		for(i32 n = 0; n < len; ++n) {
			AUDIO.lowpass_acc += ((i32)*b - AUDIO.lowpass_acc) >> AUDIO.lowpass;
			*b = (i16)AUDIO.lowpass_acc;
		}
	}
}

static void
aud_cmds_flush(void)
{
	while(AUDIO.i_cmd_r != AUDIO.i_cmd_w) {
		struct aud_cmd aud_cmd = AUDIO.cmds[AUDIO.i_cmd_r];
		AUDIO.i_cmd_r          = aud_cmd_next_index(AUDIO.i_cmd_r);
		switch(aud_cmd.type) {
		case AUD_CMD_SND_PLAY: {
			struct aud_cmd_snd_play *cmd = &aud_cmd.c.snd_play;
			for(i32 i = 0; i < NUM_SND_CHANNEL; i++) {
				struct sfx_channel *ch = &AUDIO.sfx_channel[i];
				struct adpcm *adpcm    = &ch->adpcm;
				if(ch->snd_id) continue;

				ch->snd_id      = cmd->id;
				adpcm->data     = cmd->snd.buf;
				adpcm->len      = cmd->snd.len;
				adpcm->vol_q8   = cmd->vol_q8;
				adpcm->data_pos = 0;
				adpcm_set_pitch(adpcm, cmd->pitch_q8);
				adpcm_reset_to_start(adpcm);
				break;
			}
		} break;
		case AUD_CMD_SND_MODIFY: {
			struct aud_cmd_snd_modify *cmd  = &aud_cmd.c.snd_modify;
			struct sfx_channel *sfx_channel = NULL;
			for(u32 i = 0; i < NUM_SND_CHANNEL; i++) {
				struct sfx_channel *curr = &AUDIO.sfx_channel[i];
				if(curr->snd_id == cmd->id) {
					sfx_channel = curr;
					break;
				}
			}

			if(sfx_channel == NULL) break;

			if(cmd->stop) {
				sfx_channel_stop(sfx_channel);
			}
			sfx_channel->adpcm.vol_q8 = cmd->vol_q8;
		} break;
		case AUD_CMD_MUS_PLAY: {
			struct aud_cmd_mus_play *cmd = &aud_cmd.c.mus_play;
			struct mus_channel *mc       = &AUDIO.mus_channel[0];
			mus_channel_stop(mc);
			str8 path = asset_db_get_path(&ASSETS.db, cmd->path_handle);

			void *f = sys_file_open_r(path);
			if(!f) {
				log_warn("Audio", "Can't open music file: %s", path.str);
				break;
			}

			mc->path_handle     = cmd->path_handle;
			struct adpcm *adpcm = &mc->adpcm;
			u32 num_samples     = 0;
			sys_file_r(f, &num_samples, sizeof(u32));
			adpcm->data          = mc->chunk;
			adpcm->len           = num_samples;
			adpcm->vol_q8        = cmd->vol_q8;
			mc->stream           = f;
			mc->looping          = 1;
			mc->total_bytes_file = sizeof(u32) + ((num_samples + 1) >> 1);
			adpcm_set_pitch(adpcm, 256);
			adpcm_reset_to_start(adpcm);
		} break;
		case AUD_CMD_LOWPASS: {
			struct aud_cmd_lowpass *cmd = &aud_cmd.c.lowpass;
			AUDIO.lowpass               = cmd->v;
		} break;
		default: {
			NOT_IMPLEMENTED;
		} break;
		}
	}
}
// Called by gameplay thread/context
static void
aud_push_cmd(struct aud_cmd aud_cmd)
{
	// temporary write index
	// peek new position and see if the queue is full
	u32 i = aud_cmd_next_index(AUDIO.i_cmd_w_tmp);
	sys_audio_lock();
	bool32 is_full = (i == AUDIO.i_cmd_r);
	sys_audio_unlock();

	if(is_full) { // temporary read index
		log_warn("Audio", "Queue Full!");

		// TODO: scan queue and see if we can drop a less important command
	} else {
		AUDIO.cmds[AUDIO.i_cmd_w_tmp] = aud_cmd;
		AUDIO.i_cmd_w_tmp             = i;
	}
}

static inline u32
aud_cmd_next_index(u32 i)
{
	// Wrap arround, we use & instead of %
	// beacause & is faster on power of two numbers
	return ((i + 1) & (NUM_AUD_CMD_QUEUE - 1));
}

// Called by gameplay thread/context
void
aud_cmd_queue_commit(void)
{
#ifdef PLTF_PD_HW
	// data memory barrier; prevent memory access reordering
	// ensures all commands are fully written before making them visible
	__asm("dmb");
#endif
	sys_audio_lock();
	AUDIO.i_cmd_w = AUDIO.i_cmd_w_tmp;
	sys_audio_unlock();
}

void
mus_play(const str8 path)
{
	struct aud_cmd cmd         = {.type = AUD_CMD_MUS_PLAY, .priority = AUD_CMD_PRIORITY_MUS_PLAY};
	cmd.c.mus_play.path_handle = (struct asset_handle){.path_hash = hash_string(path)};
	cmd.c.mus_play.vol_q8      = 128;
	aud_push_cmd(cmd);
}

static void
mus_channel_stop(struct mus_channel *mc)
{
	if(!mc->stream) return;
	sys_file_close(mc->stream);
	mc->stream      = NULL;
	mc->path_handle = (struct asset_handle){0};
}

static void
mus_channel_playback(struct mus_channel *mc, i16 *lb, i16 *rb, i32 len)
{
	if(!mc->stream) return;

	struct adpcm *adpcm = &mc->adpcm;
	i32 l               = min_i32(len, adpcm->len_pitched - adpcm->pos_pitched - 1);
	mus_channel_playback_part(mc, lb, rb, l);

	if(adpcm->pos_pitched < adpcm->len_pitched - 1) return;

	if(mc->looping) { // loop back to start
		adpcm_reset_to_start(adpcm);
		sys_file_seek_set(mc->stream, sizeof(u32));
		mus_channel_playback_part(mc, &lb[l], rb ? &rb[l] : NULL, len - l);
	} else {
		mus_channel_stop(mc);
	}
}

static void
mus_channel_playback_part(struct mus_channel *mc, i16 *lb, i16 *rb, i32 len)
{
	if(len <= 0) return;

	struct adpcm *adpcm = &mc->adpcm;
	adpcm->data_pos     = 0;

	assert(adpcm->pos_pitched == adpcm->pos);
	u32 pos_new = adpcm->pos + len;
	u32 bneeded = (pos_new - adpcm->pos + (adpcm->nibble == 0)) >> 1;

	if(bneeded) {
		u32 ft    = sys_file_tell(mc->stream);
		u32 fnewt = ft + bneeded;
		assert(fnewt <= mc->total_bytes_file);
		assert(bneeded <= sizeof(mc->chunk));
		sys_file_r(mc->stream, mc->chunk, bneeded);
	}

	if(adpcm->vol_q8) {
		adpcm_playback(adpcm, lb, rb, len);
	} else {
		// save some cycles on silent parallel music channels
		// adpcm_playback_nonpitch_silent(adpcm, len);
		adpcm_playback_nonpitch(adpcm, lb, rb, len);
	}
	// assert((adpcm->data_pos - data_pos_prev) == bneeded);
	assert(adpcm->pos == pos_new);
	assert(adpcm->data_pos == bneeded);
	assert(adpcm->pos_pitched == adpcm->pos);
}

struct snd
aud_load(const str8 path, struct alloc alloc)
{
	struct snd res = {0};

	void *f = sys_file_open_r(path);
	if(!f) {
		log_warn("Audio", "Can't open file for snd %s\n", path.str);
		return res;
	}

	u32 num_samples = 0;
	sys_file_r(f, &num_samples, sizeof(u32));
	u32 bytes = (num_samples + 1) >> 1;

	void *buf = alloc.allocf(alloc.ctx, bytes);
	if(!buf) {
		log_error("Audio", "Can't allocate memory for snd %s\n", path.str);
		sys_file_close(f);
		return res;
	}

	sys_file_r(f, buf, bytes);
	sys_file_close(f);

	res.buf = (u8 *)buf;
	res.len = num_samples;
	log_info("Audio", "Load aud: %s samples: %u", path.str, (uint)num_samples);
	return res;
}

static void
sfx_channel_playback(struct sfx_channel *sc, i16 *lbuf, i16 *rbuf, i32 len)
{
}
static void
sfx_channel_stop(struct sfx_channel *ch)
{
}
