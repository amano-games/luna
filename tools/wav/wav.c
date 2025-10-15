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

	struct riff_chunk_header riff_chunk_header;
	struct chunk_header chunk_header;
	struct wave_header wave_header;

	i32 format = 0, res = 0, bits_per_sample = 0, num_channels = 0;
	u32 fact_samples = 0, num_samples = 0, sample_rate = 0;

	dbg_check_warn(sys_file_r(in_file, &riff_chunk_header, sizeof(struct riff_chunk_header)), "snd-gen", "%s failed to read file");
	dbg_check_warn(riff_chunk_header.id == WAV_FOURCC('R', 'I', 'F', 'F'), "snd-gen", "%s invalid chunk_id");
	dbg_check_warn(riff_chunk_header.form_type == WAV_FOURCC('W', 'A', 'V', 'E'), "snd-gen", "%s invalid form_type");

	while(true) {
		dbg_check_warn(
			sys_file_r(in_file, &chunk_header, sizeof(struct chunk_header)),
			"snd-gen",
			"%s is not a valid .WAV file, chunk error",
			in_file_path.str);

		little_endian_to_native(&chunk_header, CHUNK_HEADER_FMT);

		if(chunk_header.id == WAV_FOURCC('f', 'm', 't', ' ')) {
			i32 supported = 1;
			dbg_check_warn(sys_file_r(in_file, &wave_header, chunk_header.size), "snd-gen", "%s failed to read fmt chunk", in_file_path.str);
			dbg_check_warn(chunk_header.size >= 16 && chunk_header.size <= sizeof(struct wave_header),
				"snd-gen",
				"%s is not a valid .WAV file! bad chunk",
				in_file_path.str);
			little_endian_to_native(&wave_header, WAVE_HEADER_FMT);

			format = (wave_header.format_tag == WAVE_FORMAT_EXTENSIBLE && chunk_header.size == 40) ? wave_header.sub_format : wave_header.format_tag;
			dbg_check_warn(
				format == WAVE_FORMAT_PCM,
				"snd-gen",
				"%s is not a supported .WAV format:%d",
				in_file_path.str,
				format);

			bits_per_sample = (chunk_header.size == 40 && wave_header.samples.valid_bits_per_sample) ? wave_header.samples.valid_bits_per_sample : wave_header.bits_per_sample;

			dbg_check_warn(wave_header.num_channels > 0, "snd-gen", "%s invalid, wrong number of channels", in_file_path.str, wave_header.num_channels);
			dbg_check_warn(wave_header.num_channels <= 2, "snd-gen", "%s invalid, wrong number of channels", in_file_path.str, wave_header.num_channels);
			dbg_check_warn(wave_header.num_channels == 1, "snd-gen", "%s invalid, unsupported stereo", in_file_path.str);
			dbg_check_warn(wave_header.sample_rate == 44100, "snd-gen", "%s invalid, sample rate unsupported: %d", in_file_path.str, wave_header.sample_rate);
			dbg_check_warn(wave_header.bits_per_sample == 16, "snd-gen", "%s invalid, 16bit per sample required got: %d", in_file_path.str, wave_header.bits_per_sample);

			// fprintf(stderr, "%d-bit ADPCM, %d channels, %d samples/block, %d-byte block alignment", bits_per_sample, wave_header.num_channels, wave_header.samples.samples_per_block, wave_header.block_align);
			// TODO: Only needed in ADPCM not WAV
			// if(wave_header.samples.samples_per_block >
			// 	adpcm_block_size_to_sample_count(wave_header.block_align, wave_header.num_channels, bits_per_sample)) {
			// 	fprintf(stderr, "%s is not a valid .WAV file, bad samples per block", in_file_path.str);
			// 	return -1;
			// }
		} else if(chunk_header.id == WAV_FOURCC('f', 'a', 'c', 't')) {
			dbg_check_warn(
				sys_file_r(in_file, &fact_samples, sizeof(fact_samples)) &&
					chunk_header.size >= 4,
				"snd-gen",
				"%s is not a valid .WAV file!, bad chunk size",
				in_file_path.str);

			little_endian_to_native(&fact_samples, "L");

			if(chunk_header.size > 4) {
				i32 bytes_to_skip = chunk_header.size - 4;
				char dummy;

				while(bytes_to_skip--) {
					dbg_check_warn(sys_file_r(in_file, &dummy, 1), "snd-gen", "%s invalid, dummy error", in_file_path.str);
				}
			}
		} else if(chunk_header.id == WAV_FOURCC('d', 'a', 't', 'a')) {
			// on the data chunk, get size and exit parsing loop

			dbg_check_warn(wave_header.num_channels > 0, "snd-gen", "%s invalid, missing fmt block", in_file_path.str);
			dbg_check_warn(chunk_header.size > 0, "snd-gen", "%s invalid, no audio samples, probably corrupt", in_file_path.str);

			if(format == WAVE_FORMAT_PCM) {
				dbg_check_warn(chunk_header.size % wave_header.block_align == 0, "snd-gen", "%s invalid, wrong block align", in_file_path.str);
				num_samples = chunk_header.size / wave_header.block_align;
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

			dbg_check_warn(num_samples > 0, "snd-gen", "%s invalid, no audio samples", in_file_path.str);

			num_channels = wave_header.num_channels;
			sample_rate  = wave_header.sample_rate;
			break;

		} else { // just ignore unknown chunks
			int bytes_to_eat = (chunk_header.size + 1) & ~1L;
			char dummy;

			// fprintf(stderr, "extra unknown chunk \"%c%c%c%c\" of %d bytes", chunk_header.chunk_id[0], chunk_header.chunk_id[1], chunk_header.chunk_id[2], chunk_header.chunk_id[3], chunk_header.size);

			while(bytes_to_eat--)
				dbg_check_warn(sys_file_r(in_file, &dummy, 1), "snd-gen", "%s invalid, failed to eat dummy", in_file_path.str);
		}
	}

	str8 out_file_path = make_file_name_with_ext(scratch, out_path, str8_lit(SND_FILE_EXT));

	dbg_check_warn((out_file = sys_file_open_w(out_file_path)), "snd-gen", "failed to open file \"%s\" for writing!", out_file_path.str);
	dbg_check_warn(sys_file_w(out_file, &num_samples, sizeof(u32)), "snd-gen", "failed to write file \"%s\"!", out_file_path.str);

	// TODO: Calc correct len
	usize data_size = num_samples * (bits_per_sample / 8);

	if(format == WAVE_FORMAT_PCM) {
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
