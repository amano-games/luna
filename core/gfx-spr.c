#include "gfx-spr.h"

#include "sys.h"
#include "gfx.h"
#include "sys-intrin.h"
#include "mathfunc.h"
#include "trace.h"

static void
spr_blit(u32 *restrict dp, u32 *restrict dm, u32 sp, u32 sm, int mode)
{
	switch(mode) {
	case SPR_MODE_INV: sp = ~sp; // fallthrough
	case SPR_MODE_COPY: *dp = (*dp & ~sm) | (sp & sm); break;
	case SPR_MODE_XOR: sp = ~sp; // fallthrough
	case SPR_MODE_NXOR: *dp = (*dp & ~sm) | ((*dp ^ sp) & sm); break;
	case SPR_MODE_WHITE_ONLY: sm &= sp; // fallthrough
	case SPR_MODE_WHITE: *dp |= sm; break;
	case SPR_MODE_BLACK_ONLY: sm &= ~sp; // fallthrough
	case SPR_MODE_BLACK: *dp &= ~sm; break;
	}

	if(dm) *dm |= sm;
}

static void
spr_blit_fwd(u32 *restrict dest_pixel_words, const u32 *restrict source_pixel_words, u32 pt, int sa, int da, int l, int r, u32 ml, u32 mr, int mode, int sm, int dm, int offs)
{
	u32 *restrict dp       = dest_pixel_words;
	const u32 *restrict sp = source_pixel_words;
	u32 p                  = 0;
	u32 m                  = 0;

	if(l == 0) { // same alignment, fast path
		p = *sp;
		if(sa == 2) {
			m = *(sp + 1) & ml;
		} else {
			m = ml;
		}
		sp += sa;
		for(int i = 0; i < dm; i++) {
			spr_blit(dp, da == 1 ? NULL : dp + 1, p, m & pt, mode);
			p = *sp;
			if(sa == 2) {
				m = *(sp + 1);
			} else {
				m = 0xFFFFFFFFU;
			}
			sp += sa;
			dp += da;
		}
		spr_blit(dp, da == 1 ? NULL : dp + 1, p, m & (pt & mr), mode);
		return;
	}

	if(0 < offs) { // first word
		p = bswap32(*sp) << l;
		if(sa == 2) {
			m = bswap32(*(sp + 1)) << l;
		} else {
			m = 0xFFFFFFFFU;
		}
		if(0 < sm) {
			sp += sa;
		}
	}

	p = bswap32(p | (bswap32(*sp) >> r));
	if(sa == 2) {
		m = bswap32(m | (bswap32(*(sp + 1)) >> r)) & ml;
	} else {
		m = ml;
	}
	if(dm == 0) {
		spr_blit(dp, da == 1 ? NULL : dp + 1, p, m & (pt & mr), mode);
		return; // only one word long
	}

	spr_blit(dp, da == 1 ? NULL : dp + 1, p, m & pt, mode);
	dp += da;

	for(int i = 1; i < dm; i++) { // middle words without first and last word
		p = bswap32(*sp) << l;
		p = bswap32(p | (bswap32(*(sp + sa)) >> r));
		if(sa == 2) {
			m = bswap32(*(sp + 1)) << l;
			m = bswap32(m | (bswap32(*(sp + 3)) >> r));
		} else {
			m = 0xFFFFFFFFU;
		}
		spr_blit(dp, da == 1 ? NULL : dp + 1, p, m & pt, mode);
		sp += sa;
		dp += da;
	}

	p = bswap32(*sp) << l; // last word
	if(sa == 2) {
		m = bswap32(*(sp + 1)) << l;
	} else {
		m = 0xFFFFFFFFU;
	}
	if(sp < source_pixel_words + sm) { // this is different for reversed blitting!
		sp += sa;
		p |= bswap32(*sp) >> r;
		if(sa == 2) {
			m |= bswap32(*(sp + 1)) >> r;
		}
	}
	p = bswap32(p);
	m = bswap32(m);
	spr_blit(dp, da == 1 ? NULL : dp + 1, p, m & (pt & mr), mode);
}

static void
spr_blit_rev(u32 *restrict dest_pixel_words, const u32 *restrict source_pixel_words, u32 pt, int sa, int da, int l, int r, u32 ml, u32 mr, int mode, int sm, int dm, int offs)
{
	u32 *restrict dp       = dest_pixel_words;
	const u32 *restrict sp = source_pixel_words + sm;
	u32 p                  = 0;
	u32 m                  = 0;

	if(l == 0) { // same alignment, fast path
		p = brev32(*sp);
		if(sa == 2) {
			m = brev32(*(sp + 1)) & ml;
		} else {
			m = ml;
		}
		sp -= sa;
		for(int i = 0; i < dm; i++) {
			spr_blit(dp, da == 1 ? NULL : dp + 1, p, m & pt, mode);
			p = brev32(*sp);
			if(sa == 2) {
				m = brev32(*(sp + 1));
			} else {
				m = 0xFFFFFFFFU;
			}
			sp -= sa;
			dp += da;
		}
		spr_blit(dp, da == 1 ? NULL : dp + 1, p, m & (pt & mr), mode);
		return;
	}

	if(0 < offs) { // first word
		p = brev32(bswap32(*sp)) << l;
		if(sa == 2) {
			m = brev32(bswap32(*(sp + 1))) << l;
		} else {
			m = 0xFFFFFFFFU;
		}
		if(0 < sm) {
			sp -= sa;
		}
	}

	p = bswap32(p | (brev32(bswap32(*sp)) >> r));
	if(sa == 2) {
		m = bswap32(m | (brev32(bswap32(*(sp + 1))) >> r)) & ml;
	} else {
		m = ml;
	}
	if(dm == 0) {
		spr_blit(dp, da == 1 ? NULL : dp + 1, p, m & (pt & mr), mode);
		return; // only one word long
	}

	spr_blit(dp, da == 1 ? NULL : dp + 1, p, m & pt, mode);
	dp += da;

	for(int i = 1; i < dm; i++) { // middle words without first and last word
		p = brev32(bswap32(*sp)) << l;
		p = bswap32(p | (brev32(bswap32(*(sp - sa))) >> r));
		if(sa == 2) {
			m = brev32(bswap32(*(sp + 1))) << l;
			m = bswap32(m | (brev32(bswap32(*(sp - 1))) >> r));
		} else {
			m = 0xFFFFFFFFU;
		}
		spr_blit(dp, da == 1 ? NULL : dp + 1, p, m & pt, mode);
		sp -= sa;
		dp += da;
	}

	p = brev32(bswap32(*sp)) << l; // last word
	if(sa == 2) {
		m = brev32(bswap32(*(sp + 1))) << l;
	} else {
		m = 0xFFFFFFFFU;
	}
	if(source_pixel_words < sp) {
		sp -= sa;
		p |= brev32(bswap32(*sp)) >> r;
		if(sa == 2) {
			m |= brev32(bswap32(*(sp + 1))) >> r;
		}
	}
	p = bswap32(p);
	m = bswap32(m);
	spr_blit(dp, da == 1 ? NULL : dp + 1, p, m & (pt & mr), mode);
}

static void
spr_blit_x(u32 *restrict dp, u32 sp, u32 sm, int mode)
{
	switch(mode) {
	case SPR_MODE_INV: sp = ~sp; // fallthrough
	case SPR_MODE_COPY: *dp = (*dp & ~sm) | (sp & sm); break;
	case SPR_MODE_XOR: sp = ~sp; // fallthrough
	case SPR_MODE_NXOR: *dp = (*dp & ~sm) | ((*dp ^ sp) & sm); break;
	case SPR_MODE_WHITE_ONLY: sm &= sp; // fallthrough
	case SPR_MODE_WHITE: *dp |= sm; break;
	case SPR_MODE_BLACK_ONLY: sm &= ~sp; // fallthrough
	case SPR_MODE_BLACK: *dp &= ~sm; break;
	}
}

// special case: drawing from masked to opaque
static void
spr_blit_rev_x(u32 *restrict dest_pixel_words, const u32 *restrict source_pixel_words, u32 pt, int l, int r, u32 ml, u32 mr, int mode, int sm, int dm, int offs)
{
	u32 *restrict dp       = dest_pixel_words;
	const u32 *restrict sp = source_pixel_words + sm;
	u32 p                  = 0;
	u32 m                  = 0;

	if(l == 0) { // same alignment, fast path
		p = brev32(*sp);
		m = brev32(*(sp + 1)) & ml;
		sp -= 2;
		for(int i = 0; i < dm; i++) {
			spr_blit_x(dp, p, m & pt, mode);
			p = brev32(*sp);
			m = brev32(*(sp + 1));
			sp -= 2;
			dp += 1;
		}
		spr_blit_x(dp, p, m & (pt & mr), mode);
		return;
	}

	if(0 < offs) { // first word
		p = brev32(bswap32(*sp)) << l;
		m = brev32(bswap32(*(sp + 1))) << l;
		if(0 < sm) {
			sp -= 2;
		}
	}

	p = bswap32(p | (brev32(bswap32(*sp)) >> r));
	m = bswap32(m | (brev32(bswap32(*(sp + 1))) >> r)) & ml;
	if(dm == 0) {
		spr_blit_x(dp, p, m & (pt & mr), mode);
		return; // only one word long
	}

	spr_blit_x(dp, p, m & pt, mode);
	dp += 1;

	for(int i = 1; i < dm; i++) { // middle words without first and last word
		p = brev32(bswap32(*sp)) << l;
		p = bswap32(p | (brev32(bswap32(*(sp - 2))) >> r));
		m = brev32(bswap32(*(sp + 1))) << l;
		m = bswap32(m | (brev32(bswap32(*(sp - 1))) >> r));
		spr_blit_x(dp, p, m & pt, mode);
		sp -= 2;
		dp += 1;
	}

	p = brev32(bswap32(*sp)) << l; // last word
	m = brev32(bswap32(*(sp + 1))) << l;
	if(source_pixel_words < sp) {
		sp -= 2;
		p |= brev32(bswap32(*sp)) >> r;
		m |= brev32(bswap32(*(sp + 1))) >> r;
	}
	p = bswap32(p);
	m = bswap32(m);
	spr_blit_x(dp, p, m & (pt & mr), mode);
}

// special case: drawing from masked to opaque
static void
spr_blit_fwd_x(
	u32 *restrict dest_pixel_words,
	const u32 *restrict source_pixel_words,
	u32 pattern,
	int word_shift_left,
	int word_shift_right,
	u32 mask_left,
	u32 mask_right,
	int mode,
	int src_words_touched,
	int dst_words_touched,
	int alignment_offset)
{
	u32 *restrict dp       = dest_pixel_words;
	const u32 *restrict sp = source_pixel_words;
	u32 p                  = 0;
	u32 m                  = 0;

	if(word_shift_left == 0) { // same alignment, fast path
		p = *sp;
		m = *(sp + 1) & mask_left;
		sp += 2;
		for(int i = 0; i < dst_words_touched; i++) {
			spr_blit_x(dp, p, m & pattern, mode);
			p = *sp;
			m = *(sp + 1);
			sp += 2;
			dp++;
		}
		spr_blit_x(dp, p, m & (pattern & mask_right), mode);
		return;
	}

	if(0 < alignment_offset) { // first word
		p = bswap32(*sp) << word_shift_left;
		m = bswap32(*(sp + 1)) << word_shift_left;
		if(0 < src_words_touched) {
			sp += 2;
		}
	}

	p = bswap32(p | (bswap32(*sp) >> word_shift_right));
	m = bswap32(m | (bswap32(*(sp + 1)) >> word_shift_right)) & mask_left;
	if(dst_words_touched == 0) {
		spr_blit_x(dp, p, m & (pattern & mask_right), mode);
		return; // only one word long
	}

	spr_blit_x(dp, p, m & pattern, mode);
	dp++;

	for(int i = 1; i < dst_words_touched; i++) { // middle words without first and last word
		p = bswap32(*sp) << word_shift_left;
		p = bswap32(p | (bswap32(*(sp + 2)) >> word_shift_right));
		m = bswap32(*(sp + 1)) << word_shift_left;
		m = bswap32(m | (bswap32(*(sp + 3)) >> word_shift_right));
		spr_blit_x(dp, p, m & pattern, mode);
		sp += 2;
		dp++;
	}

	p = bswap32(*sp) << word_shift_left; // last word
	m = bswap32(*(sp + 1)) << word_shift_left;
	if(sp < source_pixel_words + src_words_touched) { // this is different for reversed blitting!
		sp += 2;
		p |= bswap32(*sp) >> word_shift_right;
		m |= bswap32(*(sp + 1)) >> word_shift_right;
	}
	p = bswap32(p);
	m = bswap32(m);
	spr_blit_x(dp, p, m & (pattern & mask_right), mode);
}

void
gfx_spr(struct gfx_ctx ctx, struct tex_rec src, v2_i32 pos, int flip, int mode)
{
	// area bounds on canvas [x1/y1, x2/y2)
	int x1 = max_i32(pos.x, ctx.clip_x1);               // inclusive
	int y1 = max_i32(pos.y, ctx.clip_y1);               // inclusive
	int x2 = min_i32(pos.x + src.r.w - 1, ctx.clip_x2); // inclusive
	int y2 = min_i32(pos.y + src.r.h - 1, ctx.clip_y2); // inclusive
	if(x2 < x1) return;
	TRACE_START(__func__);

	struct tex dtex       = ctx.dst;
	struct tex stex       = src.t;
	bool32 fx             = (flip & SPR_FLIP_X) != 0;                                              // flipped x
	bool32 fy             = (flip & SPR_FLIP_Y) != 0;                                              // flipped y
	int sign_flip_y       = fy ? -1 : +1;                                                          // sign flip x
	int sign_flip_x       = fx ? -1 : +1;                                                          // sign flip y
	int bits_in_row       = (x2 + 1) - x1;                                                         // number of bits in a row
	int dst_obit_offset   = x1 & 31;                                                               // bitoffset in dst
	int dst_words_touched = (dst_obit_offset + bits_in_row - 1) >> 5;                              // number of touched dst words -1
	u32 mask_left         = bswap32(0xFFFFFFFFU >> (31 & dst_obit_offset));                        // mask to cut off boundary left
	u32 mask_right        = bswap32(0xFFFFFFFFU << (31 & (uint)(-dst_obit_offset - bits_in_row))); // mask to cut off boundary right
	int first_bit         = src.r.x - sign_flip_x * pos.x + (fx ? src.r.w - (x2 + 1) : x1);        // first bit index in src row
	int src_bit_offset    = (uint)(sign_flip_x * first_bit - fx * bits_in_row) & 31;               // bitoffset in src
	int dst_words_to_px   = 1 + (dtex.fmt == TEX_FMT_MASK);                                        // number of words to next logical pixel word in dst
	int src_words_to_px   = 1 + (stex.fmt == TEX_FMT_MASK);                                        // number of words to next logical pixel word in src
	int src_words_touched = ((src_bit_offset + bits_in_row - 1) >> 5) * src_words_to_px;           // number of touched src words -1
	int alginment_offset  = src_bit_offset - dst_obit_offset;                                      // alignment difference
	int word_shift_left   = alginment_offset & 31;                                                 // word left shift amount
	int word_shift_right  = 32 - word_shift_left;                                                  // word rght shift amound

	// dst pixel words
	u32 *dst_px_word = &dtex.px[((x1 >> 5) << (dtex.fmt == TEX_FMT_MASK)) + y1 * dtex.wword];
	// src pixel words
	u32 *src_pixel_words = &stex.px[(first_bit >> 5) * src_words_to_px + stex.wword * (src.r.y + sign_flip_y * (y1 - pos.y) + fy * (src.r.h - 1))];

	if(dst_words_to_px == 1 && src_words_to_px == 2) {
		for(int y = y1; y <= y2; y++, src_pixel_words += stex.wword * sign_flip_y, dst_px_word += dtex.wword) {
			if(y > 230) {
				sys_display_update_rows(0, SYS_DISPLAY_H - 1);
			}
			u32 pattern = ctx.pat.p[y & 7];
			if(pattern == 0) continue;
			if(fx) {
				spr_blit_rev_x(
					dst_px_word,
					src_pixel_words,
					pattern,
					word_shift_left,
					word_shift_right,
					mask_left,
					mask_right,
					mode,
					src_words_touched,
					dst_words_touched,
					alginment_offset);
			} else {
				spr_blit_fwd_x(
					dst_px_word,
					src_pixel_words,
					pattern,
					word_shift_left,
					word_shift_right,
					mask_left,
					mask_right,
					mode,
					src_words_touched,
					dst_words_touched,
					alginment_offset);
			}
		}
	} else {
		for(int y = y1; y <= y2; y++, src_pixel_words += stex.wword * sign_flip_y, dst_px_word += dtex.wword) {
			u32 pattern = ctx.pat.p[y & 7];
			if(pattern == 0) continue;
			if(fx) {
				spr_blit_rev(dst_px_word, src_pixel_words, pattern, src_words_to_px, dst_words_to_px, word_shift_left, word_shift_right, mask_left, mask_right, mode, src_words_touched, dst_words_touched, alginment_offset);
			} else {
				spr_blit_fwd(dst_px_word, src_pixel_words, pattern, src_words_to_px, dst_words_to_px, word_shift_left, word_shift_right, mask_left, mask_right, mode, src_words_touched, dst_words_touched, alginment_offset);
			}
		}
	}
	TRACE_END();
}
