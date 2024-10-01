#pragma once

#include "lib/mem.h"
#include "sys-types.h"

struct sound {
	u8 *buf;
	u32 len;
};

void audio_do(i16 *lbuf, i16 *rbuf, i32 len);
struct sound audio_load(const str8 path, struct alloc alloc);
