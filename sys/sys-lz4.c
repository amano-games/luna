#include "sys/sys-lz4.h"

// Decode-only LZ4 block format, bounds checked like LZ4_decompress_safe.
// https://github.com/lz4/lz4/blob/dev/doc/lz4_Block_format.md

static inline b32
sys_lz4_read_len(const u8 **ip, const u8 *iend, usize *len)
{
	u32 b = 0;
	do {
		if(*ip >= iend) { return false; }
		b = *(*ip)++;
		*len += b;
	} while(b == 255);
	return true;
}

i32
sys_lz4_decompress(const void *src, void *dst, ssize src_size, ssize dst_cap)
{
	const u8 *ip       = (const u8 *)src;
	const u8 *iend     = ip + src_size;
	u8 *op             = (u8 *)dst;
	u8 *const op_start = op;
	u8 *const oend     = op + dst_cap;

	if(src_size <= 0 || dst_cap < 0) { return -1; }

	for(;;) {
		u32 token = *ip++;

		// Literals
		usize lit = token >> 4;
		if(lit == 15 && !sys_lz4_read_len(&ip, iend, &lit)) { return -1; }
		if(lit > (usize)(iend - ip) || lit > (usize)(oend - op)) { return -1; }
		if(lit <= 16 && iend - ip >= 16 && oend - op >= 16) {
			mcpy(op, ip, 16);
		} else {
			mcpy(op, ip, lit);
		}
		op += lit;
		ip += lit;

		// Last sequence is literals only
		if(ip == iend) { break; }

		// Match
		if(iend - ip < 2) { return -1; }
		usize off = (usize)ip[0] | ((usize)ip[1] << 8);
		ip += 2;
		if(off == 0 || off > (usize)(op - op_start)) { return -1; }

		usize len = token & 15;
		if(len == 15 && !sys_lz4_read_len(&ip, iend, &len)) { return -1; }
		len += 4;
		if(len > (usize)(oend - op)) { return -1; }

		const u8 *m = op - off;
		u8 *mend    = op + len;
		if(off >= 16 && (usize)(oend - op) >= len + 15) {
			// Wild copy: may write up to 15 bytes past mend, still in bounds
			do {
				mcpy(op, m, 16);
				op += 16;
				m += 16;
			} while(op < mend);
		} else if(off >= 8 && (usize)(oend - op) >= len + 7) {
			do {
				mcpy(op, m, 8);
				op += 8;
				m += 8;
			} while(op < mend);
		} else if((usize)(oend - op) >= len + 7) {
			// Short repeating pattern: seed 8 bytes, then copy 8 at a time
			// from the nearest whole period at least 8 bytes back.
			for(usize i = 0; i < 8; ++i) { op[i] = m[i]; }
			usize d = off * ((8 + off - 1) / off);
			op += 8;
			while(op < mend) {
				mcpy(op, op - d, 8);
				op += 8;
			}
		} else {
			while(op < mend) { *op++ = *m++; }
		}
		op = mend;
	}

	return (i32)(op - op_start);
}

i32
sys_lz4_compress_bound(ssize src_size)
{
	// LZ4_COMPRESSBOUND
	if(src_size < 0 || src_size > SYS_LZ4_MAX_INPUT) { return 0; }
	return (i32)(src_size + src_size / 255 + 16);
}
