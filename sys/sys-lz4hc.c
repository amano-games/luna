#include "lz4/lz4hc.c"

#include "sys/sys-lz4hc.h"

#ifndef SYS_LZ4HC_LEVEL
#define SYS_LZ4HC_LEVEL 10
#endif

i32
sys_lz4_compress_hc(const void *src, void *dst, ssize src_size, ssize dst_cap)
{
	return LZ4_compress_HC(
		(const char *)src,
		(char *)dst,
		(int)src_size,
		(int)dst_cap,
		SYS_LZ4HC_LEVEL);
}
