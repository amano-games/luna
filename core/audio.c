#include "audio.h"
#include "debug-draw/debug-draw.h"
#include "mathfunc.h"
#include "sys-intrin.h"
#include "sys-io.h"
#include "sys-log.h"
#include "sys-utils.h"
#include "sys.h"

static void adpcm_reset_to_start(struct adpcm *pcm);
static void adpcm_set_pitch(struct adpcm *pcm, i32 pitch_q8);
static i32 adpcm_step(u8 step, i16 *history, i16 *step_size);
static i32 adpcm_advance_to(struct adpcm *pcm, u32 pos);
static i32 adpcm_advance_step(struct adpcm *pcm);
static void adpcm_playback_nonpitch_silent(struct adpcm *pcm, i32 len);
static void adpcm_playback(struct adpcm *pcm, i16 *lb, i16 *rb, i32 len);
static void adpcm_playback_non_pitch(struct adpcm *pcm, i16 *lb, i16 *rb, i32 len);

static void mus_channel_playback(struct mus_channel *mc, i16 *lb, i16 *rb, i32 len);
static void mus_channel_playback_part(struct mus_channel *mc, i16 *lb, i16 *rb, i32 len);
static void mus_channel_stop(struct mus_channel *mc);

i16 sin_wave(f32 time, f32 freq, f32 amp);
i16 square_wave(void);

void
audio_do(i16 *lbuf, i16 *rbuf, i32 len)
{

	for(usize n = 0; n < ARRLEN(AUDIO.mus_channel); n++) {
		struct mus_channel *ch = &AUDIO.mus_channel[n];
		mus_channel_playback(ch, lbuf, rbuf, len);
	}
	// const f32 two_pi = PI2_FLOAT;
	// f32 amplitude    = 1.0f;     // Volume ?
	// f32 frequency    = 440.0f;   // Default frequency (A4 - 440 Hz)
	// f32 sample_rate  = 44100.0f; // 44.1 kHz sample rate
	//
	// for(usize i = 0; i < (usize)len; i++) {
	// 	i16 wave = sin_wave(0, frequency, amplitude);
	// 	lbuf[i]  = wave;
	// 	rbuf[i]  = wave;
	// }
}

void
audio_mus_play(const str8 path)
{
	struct mus_channel *mc = &AUDIO.mus_channel[0];
	char mfile[64];

	void *f = sys_file_open_r(path);
	if(!f) {
		log_warn("Audio", "Can't open music file: %s\n", path.str);
		return;
	}
	struct adpcm *adpcm = &mc->adpcm;
	u32 num_samples     = 0;
	sys_file_r(f, &num_samples, sizeof(u32));
	adpcm->data          = mc->chunk;
	adpcm->len           = num_samples;
	adpcm->vol_q8        = 128;
	mc->stream           = f;
	mc->looping          = 1;
	mc->total_bytes_file = sizeof(u32) + ((num_samples + 1) >> 1);
	adpcm_set_pitch(adpcm, 256);
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

	struct adpcm *pcm = &mc->adpcm;
	i32 l             = min_i32(len, pcm->len_pitched - pcm->pos_pitched - 1);
	mus_channel_playback_part(mc, lb, rb, l);

	if(pcm->pos_pitched < pcm->len_pitched - 1) return;

	if(mc->looping) { // loop back to start
		adpcm_reset_to_start(pcm);
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

	// amount of new sample bytes needed for this call
	if(bneeded) {
		u32 ft    = sys_file_tell(mc->stream);
		u32 fnewt = ft + bneeded;
		assert(fnewt <= mc->total_bytes_file);
		assert(bneeded <= sizeof(mc->chunk));
		sys_file_r(mc->stream, mc->chunk, bneeded);
	}

#ifdef AUD_MUS_DEBUG
	const u32 data_pos_prev = pcm->data_pos;
#endif

	if(adpcm->vol_q8) {
		adpcm_playback(adpcm, lb, rb, len);
	} else {
		// save some cycles on silent parallel music channels
		// adpcm_playback_nonpitch_silent(pcm, len);
		adpcm_playback_non_pitch(adpcm, lb, rb, len);
	}
#ifdef AUD_MUS_DEBUG
	AUD_MUS_ASSERT((pcm->data_pos - data_pos_prev) == bneeded);
	AUD_MUS_ASSERT(pcm->pos == pos_new);
	AUD_MUS_ASSERT(pcm->data_pos == bneeded);
	AUD_MUS_ASSERT(pcm->pos_pitched == pcm->pos);
#endif
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
	log_info("Audio", "Load aud: %s samples:%d", path.str, num_samples);
	return res;
}

static void
adpcm_reset_to_start(struct adpcm *adpcm)
{
	adpcm->step_size   = 127;
	adpcm->hist        = 0;
	adpcm->nibble      = 0;
	adpcm->pos         = 0;
	adpcm->pos_pitched = 0;
	adpcm->curr_sample = 0;
	adpcm->data_pos    = 0;
	adpcm->curr_byte   = 0;
}

static void
adpcm_set_pitch(struct adpcm *adpcm, i32 pitch_q8)
{
	if(pitch_q8 <= 1) return;
	adpcm->pitch_q8    = pitch_q8;
	adpcm->ipitch_q8   = (256 << 8) / adpcm->pitch_q8;
	adpcm->len_pitched = (adpcm->len * adpcm->pitch_q8) >> 8;

	// highest position in pitched len shall not be out of bounds
	assert((((adpcm->len_pitched - 1) * adpcm->ipitch_q8) >> 8) < adpcm->len);
}

static void
adpcm_playback(struct adpcm *pcm, i16 *lb, i16 *rb, i32 len)
{
	i32 half_y = SYS_DISPLAY_H * 0.5f;
	i16 *l     = lb;

	i32 prev_y = half_y;
	debug_draw_clear();

	for(i32 i = 0; i < len; i++, l++) {
		u32 p = (++pcm->pos_pitched * pcm->ipitch_q8) >> 8;
		i32 v = (adpcm_advance_to(pcm, p) * pcm->vol_q8) >> 8;

#if 0
		{
			// Debug draw wave
			i32 x1 = max_i32(0, i - 1);
			i32 y1 = prev_y;

			i32 x2 = max_i32(0, i);
			i32 y2 = half_y + ((((f32)ssat16((i32)*l + v)) / (f32)I16_MAX) * 100.0f);

			debug_draw_line(x1, y1, x2, y2);
			prev_y = y2;
		}
#endif

		*l = ssat16((i32)*l + v);
	}
}

static void
adpcm_playback_non_pitch(struct adpcm *adpcm, i16 *lb, i16 *rb, i32 len)
{
	i16 *l = lb;
	adpcm->pos_pitched += len;
	for(i32 i = 0; i < len; i++, l++) {
		i32 v = (adpcm_advance_step(adpcm) * adpcm->vol_q8) >> 8;
		*l    = ssat16((i32)*l + v);
	}
}

static i32
adpcm_advance_to(struct adpcm *pcm, u32 pos)
{
	assert(pos < pcm->len);
	assert(pcm->pos <= pos);
	while(pcm->pos < pos) { // can't savely skip any samples with ADPCM
		adpcm_advance_step(pcm);
	}
	return pcm->curr_sample;
}

static i32
adpcm_advance_step(struct adpcm *pcm)
{
	assert(pcm->pos + 1 < pcm->len);
	pcm->pos++;
	if(!pcm->nibble) {
		pcm->curr_byte = pcm->data[pcm->data_pos++];
	}
	u32 b = (pcm->curr_byte << pcm->nibble) >> 4;
	pcm->nibble ^= 4;
	pcm->curr_sample = adpcm_step(b, &pcm->hist, &pcm->step_size);
	return pcm->curr_sample;
}

// an ADPCM function to advance the sample decoding
// github.com/superctr/adpcm/blob/master/ymb_codec.c
static i32
adpcm_step(u8 step, i16 *history, i16 *step_size)
{
	static const u8 t_step[8] = {57, 57, 57, 57, 77, 102, 128, 153};

	i32 s      = step & 7;
	i32 d      = ((1 + (s << 1)) * *step_size) >> 3;
	i32 v      = ssat16((i32)*history + ((step & 8) ? -d : +d));
	*step_size = clamp_i32((t_step[s] * *step_size) >> 6, 127, 24576);
	*history   = v;
	return v;
}

i16
sin_wave(f32 time, f32 freq, f32 amp)
{
	static uint32_t count = 0;
	i16 res               = 0;
	f32 tpc               = 44100 / freq; // ticks_per_cycle
	f32 cycles            = count / tpc;
	f32 rad               = PI2_FLOAT * cycles;
	i16 amplitude         = INT16_MAX * amp;
	res                   = amplitude * sin_f32(rad);
	count++;

	return res;
}

i16
square_wave(void)
{
	static uint32_t count = 0;
	i16 half              = INT16_MAX / 2;
	i16 res               = (count++ & (1 << 3)) ? half : -half;
	return res;
}

void
audio_debug_plot_sound(struct sound *sound, i16 *buff)
{
	usize len          = sound->len - 1;
	struct adpcm adpcm = {0};
	adpcm.data         = sound->buf;
	adpcm.len          = sound->len;
	adpcm.vol_q8       = 128;
	adpcm.data_pos     = 0;
	adpcm_set_pitch(&adpcm, 256);
	adpcm_reset_to_start(&adpcm);

	i32 l = min_i32(len, adpcm.len_pitched - adpcm.pos_pitched - 1);
	adpcm_playback(&adpcm, buff, buff, l);
}
