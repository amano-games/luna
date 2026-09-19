#pragma once

#include "base/types.h"

enum { SYS_LZ4_MAX_INPUT = 0x7E000000 }; // LZ4_MAX_INPUT_SIZE

i32 sys_lz4_decompress(const void *src, void *dst, ssize src_size, ssize dst_cap);
i32 sys_lz4_compress_bound(ssize src_size);
