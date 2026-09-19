#include "lz4/lz4.c"

#include "sys/sys-lz4.h"

i32
sys_lz4_decompress(const void *src, void *dst, ssize src_size, ssize dst_cap)
{
	return LZ4_decompress_safe((const char *)src, (char *)dst, (int)src_size, (int)dst_cap);
}

i32
sys_lz4_compress_bound(ssize src_size)
{
	return LZ4_compressBound((int)src_size);
}
