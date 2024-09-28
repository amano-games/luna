#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#include "utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../external/stb_image.h"

#define IMG_EXT ".png"
#define ANI_EXT ".lunass"

struct pixel {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t alpha;
};

struct img_header {
	unsigned int w, h;
};

void
fcopy(const char *in_path, const char *out_path)
{
	FILE *f1 = fopen(in_path, "rb");
	FILE *f2 = fopen(out_path, "wb");
	char buffer[BUFSIZ];
	size_t n;

	while((n = fread(buffer, sizeof(char), sizeof(buffer), f1)) > 0) {
		if(fwrite(buffer, n, sizeof(char), f2) != 1) {
			fprintf(stderr, "Failed to copy file \n");
		}
	}

	printf("[copy] %s -> %s\n", in_path, out_path);
}

void
gen_tex(const char *in_path, const char *out_path)
{
	int w, h, n;
	uint32_t *data = (uint32_t *)stbi_load(in_path, &w, &h, &n, 4);

	char out_file_path[FILENAME_MAX];
	change_extension(out_path, out_file_path, "tex");

	int width_aligned = (w + 31) - ((w + 31) % 32);

	uint32_t data_row = 0;
	uint32_t mask_row = 0;

	size_t row_arr_i = 0;
	size_t row_size  = 32;

	FILE *file = fopen(out_file_path, "wb");

	if(file == NULL) {
		fprintf(stderr, "Failed to open file %s\n", out_file_path);
		fclose(file);
		return;
	}

	struct img_header header = {.w = w, .h = h};

	if(fwrite(&header, sizeof(header), 1, file) != 1) {
		fprintf(stderr, "Error writing header image to file\n");
		fclose(file);
		return;
	}

	for(int y = 0; y < h; ++y) {
		for(int x = 0; x < width_aligned; ++x) {
			struct pixel pixel = {0};

			if(x < w) {
				pixel = *(struct pixel *)(data + (y * w) + (x));
			}

			int color = floorf(((pixel.red + pixel.green + pixel.blue) / 3.0f) / 255);
			int alpha = pixel.alpha;

			if(color) {
				data_row |= (1u << (31 - row_arr_i));
			} else {
				data_row &= ~(1u << (31 - row_arr_i));
			}

			if(alpha) {
				mask_row |= (1u << (31 - row_arr_i));
			} else {
				mask_row &= ~(1u << (31 - row_arr_i));
			}

			row_arr_i++;

			if(row_arr_i == row_size) {
				row_arr_i               = 0;
				uint32_t data[2]        = {to_big_endian(data_row), to_big_endian(mask_row)};
				size_t elements_written = fwrite(data, sizeof(uint32_t), 2, file);

				if(elements_written != 2) {
					fprintf(stderr, "Error writing data to file\n");
					fclose(file);
					return;
				}

				// write
				// for(int i = 0; i < 32; ++i) {
				// 	printf("%u", (data_row >> i) & 1);
				// }
				// for(int i = 0; i < 32; ++i) {
				// 	printf("%u", (mask_row >> i) & 1);
				// }
				// printf(" ");
			}
		}
		// printf("\n");
	}

	fclose(file);
	printf("[gen] %s -> %s\n", in_path, out_file_path);
	stbi_image_free(data);
}

void
gen_tex_recursive(const char *in_dir, const char *out_dir)
{
	DIR *dir;
	struct dirent *entry;
	struct stat statbuf;
	char in_path[FILENAME_MAX], out_path[FILENAME_MAX];

	dir = opendir(in_dir);
	if(dir == NULL) {
		fprintf(stderr, "Cannot open directory: %s\n", in_dir);
		return;
	}

	while((entry = readdir(dir)) != NULL) {
		snprintf(in_path, sizeof(in_path), "%s/%s", in_dir, entry->d_name);
		snprintf(out_path, sizeof(out_path), "%s/%s", out_dir, entry->d_name);

		if(stat(in_path, &statbuf) == -1) {
			perror("Error getting file information");
			continue;
		}

		if(S_ISDIR(statbuf.st_mode)) {
			if(strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
				mkdir(out_path, statbuf.st_mode);
				gen_tex_recursive(in_path, out_path);
			}
		} else {
			if(strstr(entry->d_name, IMG_EXT) != NULL) {
				gen_tex(in_path, out_path);
			} else if(strstr(entry->d_name, ANI_EXT) != NULL) {
				fcopy(in_path, out_path);
			}
		}
	}
	closedir(dir);
}

int
main(int argc, char *argv[])
{
	if(argc != 3) {
		printf("Usage: %s <in_path> <destination_path>\n", argv[0]);
		return 1;
	}

	fprintf(stderr, "Processing images ...\n");
	mkdir(argv[2], 0755);
	gen_tex_recursive(argv[1], argv[2]);

	return EXIT_SUCCESS;
}
