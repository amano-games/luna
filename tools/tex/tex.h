#pragma once

#include "mem.h"
#include "sys-types.h"

struct pixel {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t alpha;
};

struct img_header {
	unsigned int w, h;
};

void handle_texture(const str8 in_path, const str8 out_path, struct alloc scratch);
