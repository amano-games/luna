#include "tex.h"
#include "path.h"
#include "str.h"
#include "sys-io.h"
#include "sys-log.h"
#include "tools/utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void
handle_texture(const str8 in_path, const str8 out_path, struct alloc scratch)
{
	int w, h, n;
	uint32_t *data = (uint32_t *)stbi_load((char *)in_path.str, &w, &h, &n, 4);

	str8 out_file_path = make_file_name_with_ext(scratch, out_path, str8_lit("tex"));

	int width_aligned = (w + 31) - ((w + 31) % 32);

	uint32_t data_row = 0;
	uint32_t mask_row = 0;

	size_t row_arr_i = 0;
	size_t row_size  = 32;

	FILE *file = sys_file_open_w(out_file_path);

	if(file == NULL) {
		log_error("tex-gen", "Failed to open file %s", out_file_path.str);
		fclose(file);
		return;
	}

	struct img_header header = {.w = w, .h = h};

	if(fwrite(&header, sizeof(header), 1, file) != 1) {
		log_error("tex-gen", "Error writing header image to file");
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
					log_error("tex-gen", "Error writing data to file");
					fclose(file);
					return;
				}
			}
		}
	}

	fclose(file);
	log_info("tex-gen", "%s -> %s\n", in_path.str, out_file_path.str);
	stbi_image_free(data);
}
