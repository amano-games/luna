#pragma once

#include "base/mem.h"
#include "base/types.h"

struct snd_header {
	u32 sample_count;
};

struct snd {
	u8 *buf;
	u32 len;
};

struct snd snd_load(const str8 path, struct alloc alloc);
struct snd snd_load_from_mem(struct alloc alloc, void *data, ssize size);
