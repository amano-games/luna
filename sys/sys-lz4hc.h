#pragma once

#include "sys/sys-lz4.h"

i32 sys_lz4_compress_hc(const void *src, void *dst, ssize src_size, ssize dst_cap);
