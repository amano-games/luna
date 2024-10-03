#include "wav.h"
#include "str.h"
#include "tools/utils.h"
#include <stdio.h>

#define CHUNK_HEADER_FMT "4L"
#define WAVE_HEADER_FMT  "SSLLSSSSLS"

int
adpcm_block_size_to_sample_count(int block_size, int num_chans, int bps)
{
	return (block_size - num_chans * 4) / num_chans * 8 / bps + 1;
}

int
handle_wav(str8 in_path, str8 out_path)
{
	char out_file_path_buff[FILENAME_MAX];
	str8 out_file_path = str8_array_fixed(out_file_path_buff);
	change_extension((char *)out_path.str, (char *)out_file_path.str, "snd");

	FILE *in_file, *out_file;

	if(!(in_file = fopen((char *)in_path.str, "rb"))) {
		fprintf(stderr, "Failed to open file %s\n", out_file_path.str);
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
		fprintf(stderr, "\"%s\" is not a valid .WAV file!\n", in_path.str);
		return -1;
	}

	while(1) {
		if(!fread(&chunk_header, sizeof(struct chunk_header), 1, in_file)) {
			fprintf(stderr, "\"%s\" is not a valid .WAV file!\n", in_path.str);
			return -1;
		}

		little_endian_to_native(&chunk_header, CHUNK_HEADER_FMT);

		if(!strncmp(chunk_header.chunk_id, "fmt ", 4)) {
			int supported = 1;
			if(chunk_header.chunk_size < 16 || chunk_header.chunk_size > sizeof(struct wave_header) ||
				!fread(&wave_header, chunk_header.chunk_size, 1, in_file)) {
				fprintf(stderr, "\"%s\" is not a valid .WAV file! bad chunk\n", in_path.str);
				return -1;
			}
			little_endian_to_native(&wave_header, WAVE_HEADER_FMT);

			format = (wave_header.format_tag == WAVE_FORMAT_EXTENSIBLE && chunk_header.chunk_size == 40) ? wave_header.sub_format : wave_header.format_tag;
			if(format != WAVE_FORMAT_IMA_ADPCM) {
				fprintf(stderr, "[err] %s is not a supported .WAV format:%d\n", in_path.str, format);
				return -1;
			}

			bits_per_sample = (chunk_header.chunk_size == 40 && wave_header.samples.valid_bits_per_sample) ? wave_header.samples.valid_bits_per_sample : wave_header.bits_per_sample;

			if(wave_header.num_channels < 1 || wave_header.num_channels > 2) {
				fprintf(stderr, "\"%s\" is not has a wierd number [%d] of channels!\n", in_path.str, wave_header.num_channels);
				return -1;
			}

			if(wave_header.num_channels != 1) {
				fprintf(stderr, "[err] %s stereo not supported!\n", in_path.str);
				return -1;
			}

			if(wave_header.sample_rate != 44100) {
				fprintf(stderr, "[err] %s sample rate not supported! %d\n", in_path.str, wave_header.sample_rate);
				return -1;
			}

			if(wave_header.bits_per_sample != 4) {
				fprintf(stderr, "[err] %s Bits per sample not supported! %d\n", in_path.str, wave_header.bits_per_sample);
				return -1;
			}

			// fprintf(stderr, "%d-bit ADPCM, %d channels, %d samples/block, %d-byte block alignment\n", bits_per_sample, wave_header.num_channels, wave_header.samples.samples_per_block, wave_header.block_align);

			if(wave_header.samples.samples_per_block >
				adpcm_block_size_to_sample_count(wave_header.block_align, wave_header.num_channels, bits_per_sample)) {
				fprintf(stderr, "\"%s\" is not a valid .WAV file!\n", in_path.str);
				return -1;
			}
		} else if(!strncmp(chunk_header.chunk_id, "fact", 4)) {
			if(chunk_header.chunk_size < 4 || !fread(&fact_samples, sizeof(fact_samples), 1, in_file)) {
				fprintf(stderr, "\"%s\" is not a valid .WAV file!\n", in_path.str);
				return -1;
			}

			little_endian_to_native(&fact_samples, "L");

			if(chunk_header.chunk_size > 4) {
				int bytes_to_skip = chunk_header.chunk_size - 4;
				char dummy;

				while(bytes_to_skip--)
					if(!fread(&dummy, 1, 1, in_file)) {
						fprintf(stderr, "\"%s\" is not a valid .WAV file!\n", in_path.str);
						return -1;
					}
			}
		} else if(!strncmp(chunk_header.chunk_id, "data", 4)) {
			// on the data chunk, get size and exit parsing loop

			if(!wave_header.num_channels) { // make sure we saw a "fmt" chunk...
				fprintf(stderr, "\"%s\" is not a valid .WAV file!\n", in_path.str);
				return -1;
			}

			if(!chunk_header.chunk_size) {
				fprintf(stderr, "this .WAV file has no audio samples, probably is corrupt!\n");
				return -1;
			}

			u32 complete_blocks = chunk_header.chunk_size / wave_header.block_align;
			int leftover_bytes  = chunk_header.chunk_size % wave_header.block_align;
			int samples_last_block;

			num_samples = complete_blocks * wave_header.samples.samples_per_block;

			if(leftover_bytes) {
				if(leftover_bytes % (wave_header.num_channels * 4)) {
					fprintf(stderr, "\"%s\" is not a valid .WAV file!\n", in_path.str);
					return -1;
				}

				printf("data chunk has %d bytes left over for final ADPCM block\n", leftover_bytes);
				samples_last_block = ((leftover_bytes - (wave_header.num_channels * 4)) * 8) / (bits_per_sample * wave_header.num_channels) + 1;
				num_samples += samples_last_block;
			} else {
				samples_last_block = wave_header.samples.samples_per_block;

				if(fact_samples) {
					if(fact_samples < num_samples && fact_samples > num_samples - samples_last_block) {
						printf("total samples reduced %lu by FACT chunk\n", (unsigned long)(num_samples - fact_samples));
						num_samples = fact_samples;
					} else if(wave_header.num_channels == 2 && (fact_samples >>= 1) < num_samples && fact_samples > num_samples - samples_last_block) {
						printf("num samples reduced %lu by [incorrect] FACT chunk\n", (unsigned long)(num_samples - fact_samples));
						num_samples = fact_samples;
					}
				}
			}

			if(!num_samples) {
				fprintf(stderr, "this .WAV file has no audio samples, probably is corrupt!\n");
				return -1;
			}

			// printf("num samples = %lu\n", (unsigned long)num_samples);

			num_channels = wave_header.num_channels;
			sample_rate  = wave_header.sample_rate;
			break;

		} else { // just ignore unknown chunks
			int bytes_to_eat = (chunk_header.chunk_size + 1) & ~1L;
			char dummy;

			// fprintf(stderr, "extra unknown chunk \"%c%c%c%c\" of %d bytes\n", chunk_header.chunk_id[0], chunk_header.chunk_id[1], chunk_header.chunk_id[2], chunk_header.chunk_id[3], chunk_header.chunk_size);

			while(bytes_to_eat--)
				if(!fread(&dummy, 1, 1, in_file)) {
					fprintf(stderr, "\"%s\" is not a valid .WAV file!\n", in_path.str);
					return -1;
				}
		}
	}

	if(!(out_file = fopen((char *)out_file_path.str, "wb"))) {
		fprintf(stderr, "can't open file \"%s\" for writing!\n", out_file_path.str);
		return -1;
	}

	if(fwrite(&num_samples, sizeof(u32), 1, out_file) != 1) {
		fprintf(stderr, "failed to write file \"%s\"!\n", out_file_path.str);
		return -1;
	}

	char buffer[BUFSIZ];
	size_t n;

	while((n = fread(buffer, sizeof(char), sizeof(buffer), in_file)) > 0) {
		if(fwrite(buffer, n, sizeof(char), out_file) != 1) {
			fprintf(stderr, "Failed to copy file \n");
		}
	}

	fclose(out_file);
	fclose(in_file);
	printf("[aud] %s -> %s\n", in_path.str, out_file_path.str);

	return 1;
}
