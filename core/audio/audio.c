#include "audio.h"
#include "audio/adpcm.h"
#include "mathfunc.h"
#include "sys-io.h"
#include "sys-log.h"
#include "sys-types.h"
#include "sys-utils.h"

static void mus_channel_playback(struct mus_channel *mc, i16 *lb, i16 *rb, i32 len);
static void mus_channel_playback_part(struct mus_channel *mc, i16 *lb, i16 *rb, i32 len);
static void mus_channel_stop(struct mus_channel *mc);

void
audio_do(i16 *lbuf, i16 *rbuf, i32 len)
{

	for(usize n = 0; n < ARRLEN(AUDIO.mus_channel); n++) {
		struct mus_channel *ch = &AUDIO.mus_channel[n];
		mus_channel_playback(ch, lbuf, rbuf, len);
	}
}

void
audio_mus_play(const str8 path)
{
	struct mus_channel *mc = &AUDIO.mus_channel[0];

	void *f = sys_file_open_r(path);
	if(!f) {
		log_warn("Audio", "Can't open music file: %s\n", path.str);
		return;
	}

	u32 num_samples = 0;
	sys_file_r(f, &num_samples, sizeof(u32));

	struct adpcm *adpcm  = &mc->adpcm;
	adpcm->data          = mc->chunk;
	adpcm->len           = num_samples;
	adpcm->vol_q8        = 128;
	mc->stream           = f;
	mc->looping          = 1;
	mc->total_bytes_file = sizeof(u32) + ((num_samples + 1) >> 1);
	adpcm_reset_to_start(adpcm);

	log_info("Audio", "Play song %s\n", path.str);
}

static void
mus_channel_stop(struct mus_channel *mc)
{
	if(!mc->stream) return;
	sys_file_close(mc->stream);
	mc->stream = NULL;
	mclr(mc->mus_name, sizeof(mc->mus_name));
}

static void
mus_channel_playback(struct mus_channel *mc, i16 *lb, i16 *rb, i32 len)
{
	if(!mc->stream) return;

	struct adpcm *adpcm = &mc->adpcm;
	i32 l               = min_i32(len, adpcm->len - adpcm->pos - 1);
	mus_channel_playback_part(mc, lb, rb, l);

	if(adpcm->pos < adpcm->len - 1) return;

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

	u32 pos_new = adpcm->pos + len;
	// We need half the bytes because we have two nibbles per byte
	u32 bneeded = (pos_new - adpcm->pos + (adpcm->nibble == 0)) >> 1;

	// amount of new sample bytes needed for this call
	if(bneeded) {
		u32 ft    = sys_file_tell(mc->stream);
		u32 fnewt = ft + bneeded;
		assert(fnewt <= mc->total_bytes_file);
		assert(bneeded <= sizeof(mc->chunk));
		sys_file_r(mc->stream, mc->chunk, bneeded);
	}

	const u32 data_pos_prev = adpcm->data_pos;

	adpcm_playback(adpcm, lb, rb, len);
	// assert((adpcm->data_pos - data_pos_prev) == bneeded);
	// assert(adpcm->pos == pos_new);
	// assert(adpcm->data_pos == bneeded);
}

struct sound
audio_load(const str8 path, struct alloc alloc)
{
	struct sound res = {0};

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

#if 0
#define SAMPLE_RATE 44100
static i16 WRITE_BUFFER[SAMPLE_RATE * 10];
static u32 WRITE_INDEX;
static bool32 WRITE_ON;
void
do_audio_write(i16 *buff, u32 len)
{
	if(WRITE_ON) {
		str8 path   = str8_lit("adpcm-test.raw");
		i32 cpy_len = min_i32(ARRLEN(WRITE_BUFFER) - WRITE_INDEX, len);

		mcpy(WRITE_BUFFER + WRITE_INDEX, buff, sizeof(i16) * cpy_len);
		WRITE_INDEX += len;
		if(WRITE_INDEX >= ARRLEN(WRITE_BUFFER)) {
			void *f = sys_file_open_w(path);
			sys_file_w(f, WRITE_BUFFER, sizeof(WRITE_BUFFER));
			sys_file_close(f);
			WRITE_ON = false;
			sys_printf("wrote %ds of adpcm at %s", ARRLEN(WRITE_BUFFER) / SAMPLE_RATE, path.str);
		}
	}
}
#endif
