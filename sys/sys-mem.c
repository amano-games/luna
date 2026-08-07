#include "sys-mem.h"
#include "base/dbg.h"
#include "base/mem.h"

/* Over-alloc + 1-byte cookie behind the aligned pointer (Owlets-style).
 * Supports align in 1..256; project uses <= MEM_ALIGN_PD_CACHE (32). */
void *
sys_alloc_aligned_raw(ssize size, ssize align, void *(*alloc)(ssize bytes))
{
	dbg_assert(size >= 0);
	dbg_assert(align > 0);
	dbg_assert(IS_POW2(align));
	dbg_assert(align <= 256);
	/* +align gives room to bump; +1 leaves a byte for the cookie before the
	 * aligned address (align starting from raw+1). */
	u8 *raw = (u8 *)alloc(size + align + 1);
	if(!raw) {
		return NULL;
	}
	u8 *aligned = (u8 *)ALIGN_POW2((uptr)(raw + 1), (uptr)align);
	aligned[-1] = (u8)(aligned - raw); /* cookie: bytes back to raw */
	return aligned;
}

void
sys_free_aligned_raw(void *p, void (*free)(void *))
{
	if(!p) {
		return;
	}
	u8 *aligned = (u8 *)p;
	u8 *raw     = aligned - aligned[-1];
	free(raw);
}
