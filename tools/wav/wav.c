#include "wav.h"
#include "audio/adpcm.h"
#include "path.h"
#include "str.h"
#include "sys-assert.h"
#include "sys.h"
#include "tools/utils.h"
#include <stdio.h>
#include <stdlib.h>

#define CHUNK_HEADER_FMT "4L"
#define WAVE_HEADER_FMT  "SSLLSSSSLS"
#define RAW_FILE_EXT     "raw"

int
adpcm_block_size_to_sample_count(int block_size, int num_chans, int bps)
{
	return (block_size - num_chans * 4) / num_chans * 8 / bps + 1;
}

int
handle_wav(str8 in_file_path, str8 out_path, struct alloc scratch)
{

	FILE *in_file, *out_file;

	if(!(in_file = fopen((char *)in_file_path.str, "rb"))) {
		log_error("snd-gen", "Failed to open file %s", in_file_path.str);
		fclose(in_file);
		return -1;
	}

	struct riff_chunk_header riff_chunk_header;
	struct chunk_header chunk_header;
	struct wave_header wave_header;

	i32 format = 0, res = 0, bits_per_sample = 0, num_channels = 0;
	u32 fact_samples = 0, num_samples = 0, sample_rate = 0;

	if(!fread(&riff_chunk_header, sizeof(struct riff_chunk_header), 1, in_file) ||
		strncmp(riff_chunk_header.chunk_id, "RIFF", 4) ||
		strncmp(riff_chunk_header.form_type, "WAVE", 4)) {
		log_error("snd-gen", "%s is not a valid .WAV file, wrong header", in_file_path.str);
		return -1;
	}

	while(1) {
		if(!fread(&chunk_header, sizeof(struct chunk_header), 1, in_file)) {
			log_error("snd-gen", "%s is not a valid .WAV file, chunk error", in_file_path.str);
			return -1;
		}

		little_endian_to_native(&chunk_header, CHUNK_HEADER_FMT);

		if(!strncmp(chunk_header.chunk_id, "fmt ", 4)) {
			int supported = 1;
			if(chunk_header.chunk_size < 16 || chunk_header.chunk_size > sizeof(struct wave_header) ||
				!fread(&wave_header, chunk_header.chunk_size, 1, in_file)) {
				log_error("snd-gen", "%s is not a valid .WAV file! bad chunk", in_file_path.str);
				return -1;
			}
			little_endian_to_native(&wave_header, WAVE_HEADER_FMT);

			format = (wave_header.format_tag == WAVE_FORMAT_EXTENSIBLE && chunk_header.chunk_size == 40) ? wave_header.sub_format : wave_header.format_tag;
			if(format != WAVE_FORMAT_PCM) {
				log_error("snd-gen", "%s is not a supported .WAV format:%d", in_file_path.str, format);
				return -1;
			}

			bits_per_sample = (chunk_header.chunk_size == 40 && wave_header.samples.valid_bits_per_sample) ? wave_header.samples.valid_bits_per_sample : wave_header.bits_per_sample;

			if(wave_header.num_channels < 1 || wave_header.num_channels > 2) {
				log_error("snd-gen", "%s is not has a wierd number [%d] of channels!", in_file_path.str, wave_header.num_channels);
				return -1;
			}

			if(wave_header.num_channels != 1) {
				log_error("snd-gen", "%s stereo not supported!", in_file_path.str);
				return -1;
			}

			if(wave_header.sample_rate != 44100) {
				log_error("snd-gen", "%s sample rate not supported! %d", in_file_path.str, wave_header.sample_rate);
				return -1;
			}

			if(wave_header.bits_per_sample != 16) {
				log_error("snd-gen", "%s Bits per sample not supported! %d", in_file_path.str, wave_header.bits_per_sample);
				return -1;
			}

			// fprintf(stderr, "%d-bit ADPCM, %d channels, %d samples/block, %d-byte block alignment", bits_per_sample, wave_header.num_channels, wave_header.samples.samples_per_block, wave_header.block_align);
			// TODO: Only needed in ADPCM not WAV
			// if(wave_header.samples.samples_per_block >
			// 	adpcm_block_size_to_sample_count(wave_header.block_align, wave_header.num_channels, bits_per_sample)) {
			// 	fprintf(stderr, "%s is not a valid .WAV file, bad samples per block", in_file_path.str);
			// 	return -1;
			// }
		} else if(!strncmp(chunk_header.chunk_id, "fact", 4)) {
			if(chunk_header.chunk_size < 4 || !fread(&fact_samples, sizeof(fact_samples), 1, in_file)) {
				log_error("snd-gen", "%s is not a valid .WAV file!, bad chunk size", in_file_path.str);
				return -1;
			}

			little_endian_to_native(&fact_samples, "L");

			if(chunk_header.chunk_size > 4) {
				int bytes_to_skip = chunk_header.chunk_size - 4;
				char dummy;

				while(bytes_to_skip--)
					if(!fread(&dummy, 1, 1, in_file)) {
						log_error("snd-gen", "%s is not a valid .WAV file, dummy error", in_file_path.str);
						return -1;
					}
			}
		} else if(!strncmp(chunk_header.chunk_id, "data", 4)) {
			// on the data chunk, get size and exit parsing loop

			if(!wave_header.num_channels) { // make sure we saw a "fmt" chunk...
				log_error("snd-gen", "%s is not a valid .WAV file!", in_file_path.str);
				return -1;
			}

			if(!chunk_header.chunk_size) {
				log_error("snd-gen", "%s this .WAV file has no audio samples, probably is corrupt!", in_file_path.str);
				return -1;
			}

			if(format == WAVE_FORMAT_PCM) {
				if(chunk_header.chunk_size % wave_header.block_align) {
					log_error("snd-gen", "%s is not a valid .WAV file!", in_file_path.str);
					return -1;
				}

				num_samples = chunk_header.chunk_size / wave_header.block_align;
			} else {
				NOT_IMPLEMENTED;
				// TODO: Handle adpcm?
#if 0
				u32 complete_blocks = chunk_header.chunk_size / wave_header.block_align;
				int leftover_bytes  = chunk_header.chunk_size % wave_header.block_align;
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

			if(!num_samples) {
				log_error("snd-gen", "%s this .WAV file has no audio samples, probably is corrupt!", in_file_path.str);
				return -1;
			}

			// printf("num samples = %lu", (unsigned long)num_samples);

			num_channels = wave_header.num_channels;
			sample_rate  = wave_header.sample_rate;
			break;

		} else { // just ignore unknown chunks
			int bytes_to_eat = (chunk_header.chunk_size + 1) & ~1L;
			char dummy;

			// fprintf(stderr, "extra unknown chunk \"%c%c%c%c\" of %d bytes", chunk_header.chunk_id[0], chunk_header.chunk_id[1], chunk_header.chunk_id[2], chunk_header.chunk_id[3], chunk_header.chunk_size);

			while(bytes_to_eat--)
				if(!fread(&dummy, 1, 1, in_file)) {
					log_error("snd-gen", "%s is not a valid .WAV file!", in_file_path.str);
					return -1;
				}
		}
	}

	str8 out_file_path = make_file_name_with_ext(scratch, out_path, str8_lit(SND_FILE_EXT));

	if(!(out_file = fopen((char *)out_file_path.str, "wb"))) {
		log_error("snd-gen", "can't open file \"%s\" for writing!", out_file_path.str);
		return -1;
	}

	if(fwrite(&num_samples, sizeof(u32), 1, out_file) != 1) {
		log_error("snd-gen", "failed to write file \"%s\"!", out_file_path.str);
		return -1;
	}

	// TODO: Calc correct len
	usize data_size = num_samples * (bits_per_sample / 8);

	if(format == WAVE_FORMAT_PCM) {
		//TODO: Is this ok?
		i16 *in_buffer = (i16 *)malloc(data_size + 2);
		u8 *out_buffer = (u8 *)malloc(data_size / 2);
		usize res      = fread(in_buffer, 1, data_size, in_file);
		if(res != data_size) {
			log_error("snd-gen", "error reading full file \"%s\"!", in_file_path.str);
		}

		fclose(in_file);
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
		fclose(out_file);
		sys_free(in_buffer);
		sys_free(out_buffer);

		log_info("snd-gen", "%s -> %s", in_file_path.str, out_file_path.str);

	} else {
		NOT_IMPLEMENTED;
	}

	return 1;
}
