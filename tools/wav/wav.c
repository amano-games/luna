#include "wav.h"
#include "engine/audio/adpcm.h"
#include "base/dbg.h"
#include "base/path.h"
#include "base/str.h"
#include "sys/sys-io.h"
#include "sys/sys.h"
#include <stdio.h>
#include <stdlib.h>

#define CHUNK_HEADER_FMT       "4L"
#define WAVE_HEADER_FMT        "SSLLSSSSLS"
#define RAW_FILE_EXT           "raw"
#define WAV_FOURCC(a, b, c, d) (uint32_t)(a | (b << 8) | (c << 16) | (d << 24))

static inline void
little_endian_to_native(void *data, char *format)
{
	unsigned char *cp = (unsigned char *)data;
	int32_t temp;

	while(*format) {
		switch(*format) {
		case 'L':
			temp           = cp[0] + ((int32_t)cp[1] << 8) + ((int32_t)cp[2] << 16) + ((int32_t)cp[3] << 24);
			*(int32_t *)cp = temp;
			cp += 4;
			break;

		case 'S':
			temp         = cp[0] + (cp[1] << 8);
			*(short *)cp = (short)temp;
			cp += 2;
			break;

		default:
			if(char_is_digit((unsigned char)*format, 10))
				cp += *format - '0';

			break;
		}

		format++;
	}
}

static inline i32
adpcm_block_size_to_sample_count(int block_size, int num_chans, int bps)
{
	return (block_size - num_chans * 4) / num_chans * 8 / bps + 1;
}

b32
handle_wav(str8 in_file_path, str8 out_path, struct alloc scratch)
{

	void *in_file  = NULL;
	void *out_file = NULL;

	dbg_check_warn((in_file = sys_file_open_r(in_file_path)), "snd-gen", "Failed to open file %s", in_file_path.str);

	struct riff_chunk chunk      = {0};
	struct wav_riff wav_riff     = {0};
	struct wav_format wav_format = {0};
	struct wav wav               = {0};
	u32 fact_samples             = 0;

	dbg_check_warn(sys_file_r(in_file, &chunk, sizeof(chunk)), "snd-gen", "%s failed to read file", in_file_path.str);
	dbg_check_warn(chunk.id == WAV_FOURCC('R', 'I', 'F', 'F'), "snd-gen", "%s invalid chunk_id", in_file_path.str);

	dbg_check_warn(sys_file_r(in_file, &wav_riff, sizeof(wav_riff)), "snd-gen", "%s failed to read file", in_file_path.str);
	dbg_check_warn(wav_riff.form_type == WAV_FOURCC('W', 'A', 'V', 'E'), "snd-gen", "%s invalid form_type", in_file_path.str);

	while(true) {
		dbg_check_warn(sys_file_r(in_file, &chunk, sizeof(chunk)), "snd-gen", "%s failed to read chunk", in_file_path.str);

		little_endian_to_native(&chunk, CHUNK_HEADER_FMT);

		if(chunk.id == WAV_FOURCC('f', 'm', 't', ' ')) {
			i32 supported = 1;
			dbg_check_warn(sys_file_r(in_file, &wav_format, chunk.size), "snd-gen", "%s failed to read fmt chunk", in_file_path.str);
			dbg_check_warn(chunk.size >= 16 && chunk.size <= sizeof(struct wav_format),
				"snd-gen",
				"%s is not a valid .WAV file! bad chunk",
				in_file_path.str);
			little_endian_to_native(&wav_format, WAVE_HEADER_FMT);

			wav.sample_format = (wav_format.format == WAVE_FORMAT_EXTENSIBLE && chunk.size == 40) ? wav_format.sub_format : wav_format.format;
			dbg_check_warn(
				wav.sample_format == WAVE_FORMAT_PCM,
				"snd-gen",
				"%s is not a supported .WAV format:%d",
				in_file_path.str,
				wav.sample_format);

			wav.sample_data_size = (chunk.size == 40 && wav_format.samples.valid_bits_per_sample) ? wav_format.samples.valid_bits_per_sample : wav_format.bits_per_sample;

			dbg_check_warn(wav_format.channel_count > 0, "snd-gen", "%s invalid, wrong number of channels", in_file_path.str, wav_format.channel_count);
			dbg_check_warn(wav_format.channel_count <= 2, "snd-gen", "%s invalid, wrong number of channels", in_file_path.str, wav_format.channel_count);
			dbg_check_warn(wav_format.channel_count == 1, "snd-gen", "%s invalid, unsupported stereo", in_file_path.str);
			dbg_check_warn(wav_format.sample_rate == 44100, "snd-gen", "%s invalid, sample rate unsupported: %d", in_file_path.str, wav_format.sample_rate);
			dbg_check_warn(wav_format.bits_per_sample == 16, "snd-gen", "%s invalid, 16bit per sample required got: %d", in_file_path.str, wav_format.bits_per_sample);

			// fprintf(stderr, "%d-bit ADPCM, %d channels, %d samples/block, %d-byte block alignment", bits_per_sample, wave_header.num_channels, wave_header.samples.samples_per_block, wave_header.block_align);
			// TODO: Only needed in ADPCM not WAV
			// if(wave_header.samples.samples_per_block >
			// 	adpcm_block_size_to_sample_count(wave_header.block_align, wave_header.num_channels, bits_per_sample)) {
			// 	fprintf(stderr, "%s is not a valid .WAV file, bad samples per block", in_file_path.str);
			// 	return -1;
			// }
		} else if(chunk.id == WAV_FOURCC('f', 'a', 'c', 't')) {
			dbg_check_warn(chunk.size == 4, "snd-gen", "%s invalid, wrong fact chunk size", in_file_path.str);
			dbg_check_warn(sys_file_r(in_file, &fact_samples, sizeof(fact_samples)), "snd-gen", "%s failed to read fact_samples", in_file_path.str);
			little_endian_to_native(&fact_samples, "L");
		} else if(chunk.id == WAV_FOURCC('d', 'a', 't', 'a')) {
			// on the data chunk, get size and exit parsing loop

			dbg_check_warn(wav_format.channel_count > 0, "snd-gen", "%s invalid, missing fmt block", in_file_path.str);
			dbg_check_warn(chunk.size > 0, "snd-gen", "%s invalid, no audio samples, probably corrupt", in_file_path.str);

			if(wav.sample_format == WAVE_FORMAT_PCM) {
				dbg_check_warn(chunk.size % wav_format.block_size == 0, "snd-gen", "%s invalid, wrong block align", in_file_path.str);
				wav.sample_count = chunk.size / wav_format.block_size;
			} else {
				dbg_not_implemeneted("snd-gen");
				// TODO: Handle adpcm?
#if 0
				u32 complete_blocks = chunk_header.size / wave_header.block_align;
				int leftover_bytes  = chunk_header.size % wave_header.block_align;
				int samples_last_block;

				num_samples = complete_blocks * wave_header.samples.samples_per_block;

				if(leftover_bytes) {
					if(leftover_bytes % (wave_header.num_channels * 4)) {
						fprintf(stderr, "\"%s\" is not a valid .WAV file!", in_file_path.str);
						return -1;
					}

					printf("data chunk has %d bytes left over for final ADPCM block", leftover_bytes);
					samples_last_block = ((leftover_bytes - (wave_header.num_channels * 4)) * 8) / (bits_per_sample * wave_header.num_channels) + 1;
					num_samples += samples_last_block;
				} else {
					samples_last_block = wave_header.samples.samples_per_block;

					if(fact_samples) {
						if(fact_samples < num_samples && fact_samples > num_samples - samples_last_block) {
							printf("total samples reduced %lu by FACT chunk", (unsigned long)(num_samples - fact_samples));
							num_samples = fact_samples;
						} else if(wave_header.num_channels == 2 && (fact_samples >>= 1) < num_samples && fact_samples > num_samples - samples_last_block) {
							printf("num samples reduced %lu by [incorrect] FACT chunk", (unsigned long)(num_samples - fact_samples));
							num_samples = fact_samples;
						}
					}
				}
#endif
			}

			dbg_check_warn(wav.sample_count > 0, "snd-gen", "%s invalid, no audio samples", in_file_path.str);

			wav.channel_count = wav_format.channel_count;
			wav.sample_rate   = wav_format.sample_rate;
			break;

		} else { // just ignore unknown chunks
			i32 bytes_to_eat = (chunk.size + 1) & ~1L;
			u8 dummy         = 0;
			while(bytes_to_eat--) {
				dbg_check_warn(sys_file_r(in_file, &dummy, 1), "snd-gen", "%s invalid, failed to eat dummy", in_file_path.str);
			}
		}
	}

	str8 out_file_path = make_file_name_with_ext(scratch, out_path, str8_lit(SND_FILE_EXT));

	dbg_check_warn((out_file = sys_file_open_w(out_file_path)), "snd-gen", "failed to open file \"%s\" for writing!", out_file_path.str);
	dbg_check_warn(sys_file_w(out_file, &wav.sample_count, sizeof(wav.sample_count)), "snd-gen", "failed to write file \"%s\"!", out_file_path.str);

	// TODO: Calc correct len
	usize data_size = wav.sample_count * (wav.sample_data_size / 8);

	if(wav.sample_format == WAVE_FORMAT_PCM) {
		//TODO: Is this ok?
		i16 *in_buffer = (i16 *)sys_alloc(NULL, data_size + 2);
		u8 *out_buffer = (u8 *)sys_alloc(NULL, data_size / 2);
		dbg_check_warn(sys_file_r(in_file, in_buffer, data_size), "snd-gen", "error reading full file \"%s\"!", in_file_path.str);
		usize length = data_size / 2;

		// WARN: Don't know where I got that I needed make the size even,
		// but this caused a buffer overflow because the in_buffer doesn't take that in to account
		// https://github.com/superctr/adpcm/blob/7736b178f4fb722d594c6ebdfc1ddf1af2ec81f7/adpcm.c#L156
		if(length & 1) length++; // make the size even
		adpcm_encode(in_buffer, out_buffer, length);
		length /= 2;

		for(usize i = 0; i < length; i++) {
			putc(*(out_buffer + i), out_file);
		}
		sys_file_close(in_file);
		sys_file_close(out_file);
		sys_free(in_buffer);
		sys_free(out_buffer);

		log_info("snd-gen", "%s -> %s", in_file_path.str, out_file_path.str);

	} else {
		dbg_not_implemeneted("snd-gen");
	}

	return true;

error:
	if(in_file) { sys_file_close(in_file); }
	if(out_file) { sys_file_close(out_file); }
	return false;
}
