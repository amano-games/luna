#include "tex.h"
#include "tools/utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void
handle_texture(const str8 in_path, const str8 out_path)
{
	int w, h, n;
	uint32_t *data = (uint32_t *)stbi_load((char *)in_path.str, &w, &h, &n, 4);

	char out_file_path[FILENAME_MAX];
	change_extension((char *)out_path.str, out_file_path, "tex");

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
	// printf("[tex] %s -> %s\n", in_path.str, out_file_path);
	stbi_image_free(data);
}
