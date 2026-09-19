#include "lz4/lz4hc.c"

#include "sys/sys-lz4hc.h"

i32
sys_lz4_compress_hc(const void *src, void *dst, ssize src_size, ssize dst_cap)
{
	return LZ4_compress_HC(
		(const char *)src,
		(char *)dst,
		(int)src_size,
		(int)dst_cap,
		LZ4HC_CLEVEL_MAX);
}
