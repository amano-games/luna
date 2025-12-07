#include "mem.h"

// // returns a pointer aligned to the next word address
// void *
// align_up_ptr(void *p)
// {
// 	return (void *)(((uptr)p + (uptr)3) & ~(uptr)3);
// }
//
// // returns a pointer aligned to the prev word address
// void *
// align_down_ptr(void *p)
// {
// 	return (void *)((uptr)p & ~(uptr)3);
// }
//
// // rounds up to the next multiple of word size
// usize
// align_up_size_t(usize p)
// {
// 	return ((p + (usize)3) & ~(usize)3);
// }
//
// // rounds up to the prev multiple of word size
// usize
// align_down_size_t(usize p)
// {
// 	return (p & ~(usize)3);
// }

// places the span beginning at the next word address; size gets adjusted
// mspan
// mspan_align(mspan m)
// {
// 	void *p0     = align_up_ptr(m.p);
// 	void *p1     = (char *)m.p + m.size;
// 	usize s      = (usize)((char *)p1 - (char *)p0);
// 	mspan result = {(void *)p0, s};
// 	return result;
// }
