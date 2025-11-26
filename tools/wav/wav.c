#include "wav.h"
#include "engine/audio/adpcm.h"
#include "base/dbg.h"
#include "base/path.h"
#include "base/str.h"
#include "engine/audio/snd.h"
#include "sys/sys-io.h"
#include "sys/sys.h"

#define WAV_FOURCC(a, b, c, d) (u32)(a | (b << 8) | (c << 16) | (d << 24))

b32
wav_parse(const void *data, ssize data_size, struct wav *res)
{
	struct wav_format *format = NULL;
	struct riff_chunk *chunk  = (struct riff_chunk *)data;
	dbg_check_warn(chunk->id == WAV_FOURCC('R', 'I', 'F', 'F'), "wav", "invalid, wrong chunk_id");

	struct wav_riff *wav_riff = (struct wav_riff *)&chunk[1];
	dbg_check_warn(wav_riff->form_type == WAV_FOURCC('W', 'A', 'V', 'E'), "wav", "invalid, wrong form_type");

	chunk = (struct riff_chunk *)&wav_riff[1];

	while(true) {
		if(chunk->id == WAV_FOURCC('f', 'm', 't', ' ')) {
			dbg_check_warn(chunk->size >= 16, "wav", "invalid, bad fmt chunk");
			dbg_check_warn(chunk->size <= sizeof(struct wav_format), "wav", "invalid, bad fmt chunk");

			format = (struct wav_format *)&chunk[1];
			if(format->format == WAVE_FORMAT_IMA_ADPCM && chunk->size == 20) {
				res->samples_per_block = ((u16 *)&format[1])[1];
			}
		} else if(chunk->id == WAV_FOURCC('f', 'a', 'c', 't')) {
			dbg_check_warn(chunk->size == 4, "wav", "invalid, wrong fact chunk");
			res->sample_count = *(u32 *)&chunk[1];
		} else if(chunk->id == WAV_FOURCC('d', 'a', 't', 'a')) {
			res->sample_data_size = chunk->size;
			break;
		}
		chunk = (struct riff_chunk *)((u8 *)&chunk[1] + chunk->size);
	}

	dbg_check_warn(format != NULL, "wav", "invalid, missing fmt block");

	res->sample_data     = &chunk[1];
	res->bits_per_sample = format->bits_per_sample;
	res->sample_format   = format->format;
	res->channel_count   = format->channel_count;
	res->sample_rate     = format->sample_rate;
	res->block_size      = format->block_size;

	if(res->sample_count == 0) {
		// No fact chunk, calculate sample count manualy
		if(res->block_size != 0 && res->samples_per_block != 0) {
			u32 num_blocks    = res->sample_data_size / res->block_size + 1;
			res->sample_count = res->samples_per_block * num_blocks;
		} else {
			res->sample_count = (int)((u64)(res->sample_data_size / res->channel_count) * 8 / res->bits_per_sample);
		}
	}

	dbg_check_warn(res->sample_count > 0, "wav", "%s invalid, no audio samples");

	return true;

error:;
	return false;
}

b32
wav_to_snd(str8 in_path, str8 out_path, struct alloc scratch)
{
	b32 res                          = false;
	struct sys_full_file_res in_data = sys_load_full_file(in_path, sys_allocator());
	void *out_file                   = NULL;
	u8 *out_data                     = NULL;
	struct wav wav                   = {0};

	dbg_check_warn(in_data.size >= 44 && in_data.data != NULL, "snd-gen", "%s failed to read full file", in_path.str);
	dbg_check_warn(wav_parse(in_data.data, in_data.size, &wav), "snd-gen", "%s failed to parse wav file", in_path.str);
	dbg_check_warn(wav.sample_format == WAVE_FORMAT_PCM, "snd_gen", "invalid, wav format not supported");
	dbg_check_warn(wav.channel_count == 1, "wav", "invalid, unsupported number of channels", wav.channel_count);
	dbg_check_warn(wav.sample_rate == 44100, "wav", "invalid, unsupported sample rate: %d", wav.sample_rate);
	dbg_check_warn(wav.bits_per_sample == 16, "wav", "invalid, unsupported bits per sample: %d", wav.bits_per_sample);

	struct snd_header snd_header = {.sample_count = wav.sample_count};
	str8 out_file_path           = path_make_file_name_with_ext(scratch, out_path, str8_lit(SND_FILE_EXT));
	dbg_check_warn((out_file = sys_file_open_w(out_file_path)), "snd-gen", "%s failed to open", out_file_path.str);
	dbg_check_warn(sys_file_w(out_file, &snd_header, sizeof(snd_header)), "snd-gen", "%s failed to write", out_file_path.str);

	{
		// The ADPCM encoder writes half a byte per sample 4bits per sample
		usize data_size = ((wav.sample_count * wav.channel_count) + 1) / 2;
		out_data        = (u8 *)sys_alloc(NULL, data_size * sizeof(u8));

		adpcm_i16_encode((i16 *)wav.sample_data, out_data, wav.sample_count);
		dbg_check_warn(sys_file_w(out_file, out_data, data_size), "snd-gen", "%s failed to write", out_file_path.str);
	}

	log_info("snd-gen", "%s -> %s", in_path.str, out_file_path.str);
	res = true;

error:;
	if(in_data.data) { sys_free(in_data.data); }
	if(out_data) { sys_free(out_data); }
	if(out_file) { sys_file_close(out_file); }
	return res;
}
