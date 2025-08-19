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

	for(usize n = 1; n < ARRLEN(AUDIO.mus_channel); n++) {
		struct mus_channel *ch = &AUDIO.mus_channel[n];
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

				ch->snd_id       = cmd->id;
				ch->repeat_count = cmd->repeat_count;
				ch->count        = 0;
				adpcm->data      = cmd->snd.buf;
				adpcm->len       = cmd->snd.len;
				adpcm->vol_q8    = cmd->vol_q8;
				adpcm->data_pos  = 0;
				adpcm_set_pitch(adpcm, cmd->pitch_q8);
				adpcm_reset_to_start(adpcm);
				break;
			}
		} break;
		case AUD_CMD_SND_STOP: {
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

			sfx_channel_stop(sfx_channel);
		case AUD_CMD_SND_MODIFY_VOL: {
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

			sfx_channel->adpcm.vol_q8 = cmd->vol_q8;
		} break;
		case AUD_CMD_SND_MODIFY_REPEAT_COUNT: {
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

			sfx_channel->repeat_count = cmd->repeat_count;
		} break;
		} break;
		case AUD_CMD_MUS_PLAY: {
			struct aud_cmd_mus_play *cmd = &aud_cmd.c.mus_play;
			dbg_assert(cmd->channel_id != AUD_MUS_CHANNEL_NONE);
			struct mus_channel *mc = &AUDIO.mus_channel[cmd->channel_id];
			mus_channel_stop(mc);
			str8 path = asset_db_path_get(&ASSETS.db, cmd->path_handle);
			if(path.size == 0) {
				log_warn("Audio", "Music file path doesn't exist in DB");
				break;
			}

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
			mc->looping          = cmd->loop;
			mc->total_bytes_file = sizeof(u32) + ((num_samples + 1) >> 1);
			adpcm_set_pitch(adpcm, 256);
			adpcm_reset_to_start(adpcm);
		} break;
		case AUD_CMD_MUS_MODIFY_VOL: {
			struct aud_cmd_mus_modify *cmd = &aud_cmd.c.mus_modify;
			struct mus_channel *mc         = &AUDIO.mus_channel[cmd->channel_id];
			mc->adpcm.vol_q8               = cmd->vol_q8;
		} break;
		case AUD_CMD_LOWPASS: {
			struct aud_cmd_lowpass *cmd = &aud_cmd.c.lowpass;
			AUDIO.lowpass               = cmd->v;
		} break;
		default: {
			dbg_not_implemeneted("Audio");
		} break;
		}
	}

error:
	return;
}
// Called by gameplay thread/context
static void
aud_push_cmd(struct aud_cmd aud_cmd)
{
	// temporary write index
	// peek new position and see if the queue is full
	u32 i = aud_cmd_next_index(AUDIO.i_cmd_w_tmp);
	sys_audio_lock();
	b32 is_full = (i == AUDIO.i_cmd_r);
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
	// to the audio context via the write index
	// -> needed because of interrupts
	__asm volatile("dmb");
#endif
	sys_audio_lock();
	AUDIO.i_cmd_w = AUDIO.i_cmd_w_tmp;
	sys_audio_unlock();
}

void
mus_play(
	const struct asset_handle handle,
	enum mus_channel_id channel_id,
	f32 vol,
	b32 loop)
{
	dbg_assert(channel_id != AUD_MUS_CHANNEL_NONE);
	struct aud_cmd cmd = {
		.type     = AUD_CMD_MUS_PLAY,
		.priority = AUD_CMD_PRIORITY_MUS_PLAY,
	};

	cmd.c.mus_play.path_handle = handle;
	cmd.c.mus_play.channel_id  = channel_id;
	cmd.c.mus_play.vol_q8      = (i32)(vol * 255.0f);
	cmd.c.mus_play.loop        = loop;
	aud_push_cmd(cmd);
}

void
mus_play_by_path(
	const str8 path,
	enum mus_channel_id channel_id,
	f32 vol,
	b32 loop)
{
	dbg_assert(channel_id != AUD_MUS_CHANNEL_NONE);
	log_info("Audio", "play music %s", path.str);
	struct asset_handle handle = (struct asset_handle){
		.path_hash = hash_string(path),
	};
	mus_play(handle, channel_id, vol, loop);
}

b32
mus_is_playing(enum mus_channel_id channel_id)
{
	dbg_assert(channel_id != AUD_MUS_CHANNEL_NONE);
	struct mus_channel *mc = &AUDIO.mus_channel[channel_id];
	return mc->stream != NULL;
}

void
mus_stop(enum mus_channel_id channel_id)
{
	dbg_assert(channel_id != AUD_MUS_CHANNEL_NONE);
	struct mus_channel *mc = &AUDIO.mus_channel[channel_id];
	mus_channel_stop(mc);
}

void
mus_set_vol(enum mus_channel_id channel_id, f32 vol)
{
	struct aud_cmd cmd          = {.type = AUD_CMD_MUS_MODIFY_VOL};
	cmd.c.mus_modify.channel_id = channel_id;
	cmd.c.mus_modify.vol_q8     = (i32)(vol * 255.0f);
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

	dbg_assert(adpcm->pos_pitched == adpcm->pos);
	u32 pos_new = adpcm->pos + len;
	u32 bneeded = (pos_new - adpcm->pos + (adpcm->nibble == 0)) >> 1;

	if(bneeded) {
		u32 ft    = sys_file_tell(mc->stream);
		u32 fnewt = ft + bneeded;
		dbg_assert(fnewt <= mc->total_bytes_file);
		dbg_assert(bneeded <= sizeof(mc->chunk));
		sys_file_r(mc->stream, mc->chunk, bneeded);
	}

	if(adpcm->vol_q8) {
		adpcm_playback(adpcm, lb, rb, len);
	} else {
		// save some cycles on silent parallel music channels
		// adpcm_playback_nonpitch_silent(adpcm, len);
		adpcm_playback_nonpitch(adpcm, lb, rb, len);
	}
	// dbg_assert((adpcm->data_pos - data_pos_prev) == bneeded);
	dbg_assert(adpcm->pos == pos_new);
	dbg_assert(adpcm->data_pos == bneeded);
	dbg_assert(adpcm->pos_pitched == adpcm->pos);
}

u32
snd_instance_play(struct snd snd, f32 vol, f32 pitch, u16 repeat_count)
{
	if(AUDIO.snd_playing_disabled) return 0;
	if(!snd.buf || snd.len == 0) return 0;

	i32 vol_q8   = (i32)(vol * 256.5f);
	i32 pitch_q8 = (i32)(pitch * 256.5f);
	if(vol_q8 == 0) return 0;

	AUDIO.snd_id++;
	if(AUDIO.snd_id == 0) {
		AUDIO.snd_id = 1;
	}

	struct aud_cmd cmd          = {.type = AUD_CMD_SND_PLAY};
	cmd.c.snd_play.snd          = snd;
	cmd.c.snd_play.id           = AUDIO.snd_id;
	cmd.c.snd_play.pitch_q8     = pitch_q8;
	cmd.c.snd_play.vol_q8       = vol_q8;
	cmd.c.snd_play.repeat_count = repeat_count;
	aud_push_cmd(cmd);
	return AUDIO.snd_id;
}

void
snd_instance_stop(u32 id)
{
	struct aud_cmd cmd  = {.type = AUD_CMD_SND_STOP};
	cmd.c.snd_modify.id = id;
	aud_push_cmd(cmd);
}

void
snd_instance_set_repeat_count(u32 snd_id, u16 repeat_count)
{
	struct aud_cmd cmd            = {.type = AUD_CMD_SND_MODIFY_REPEAT_COUNT};
	cmd.c.snd_modify.id           = snd_id;
	cmd.c.snd_modify.repeat_count = repeat_count;
	aud_push_cmd(cmd);
}

void
snd_instance_set_vol(u32 snd_id, f32 vol)
{
	struct aud_cmd cmd      = {.type = AUD_CMD_SND_MODIFY_VOL};
	cmd.c.snd_modify.id     = snd_id;
	cmd.c.snd_modify.vol_q8 = (i32)(vol * 256.5f);
	aud_push_cmd(cmd);
}

static void
sfx_channel_playback(struct sfx_channel *sc, i16 *lbuf, i16 *rbuf, i32 len)
{
	if(!sc->snd_id) return;

	struct adpcm *adpcm = &sc->adpcm;
	dbg_assert(0 < adpcm->len_pitched);
	dbg_assert(adpcm->pos_pitched < adpcm->len_pitched);
	i32 l = min_i32(len, adpcm->len_pitched - adpcm->pos_pitched - 1);
	adpcm_playback(adpcm, lbuf, rbuf, l);
	dbg_assert(adpcm->pos_pitched < adpcm->len_pitched);

	if(adpcm->pos_pitched == adpcm->len_pitched - 1) {
		sc->count++;
		if(sc->repeat_count == 0 || sc->count < sc->repeat_count) {
			adpcm_reset_to_start(adpcm);
		} else {
			sc->snd_id = 0;
		}
	}
}

b32
snd_instance_is_playing(u32 snd_id)
{
	if(snd_id == 0) return false;
	for(u32 i = 0; i < NUM_SND_CHANNEL; i++) {
		struct sfx_channel *curr = &AUDIO.sfx_channel[i];
		if(curr->snd_id == snd_id) {
			return true;
		}
	}

	return false;
}

static void
sfx_channel_stop(struct sfx_channel *ch)
{
	ch->adpcm.data = NULL;
	ch->snd_id     = 0;
}
