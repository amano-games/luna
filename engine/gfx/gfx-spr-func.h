// =============================================================================
// Copyright 2025, Lukas Wolski (the.strupf@proton.me). All rights reserved.
// =============================================================================

// template for a rectangular sprite blitting function

// user input for inclusion:
// ---
// #define SPRBLIT_FUNCNAME <- name of function
// #define SPRBLIT_FLIPPEDX <- 0/1: draw sprites mirrored x
// #define SPRBLIT_SRC_MASK <- 0/1: src texture has transparency
// #define SPRBLIT_DST_MASK <- 0/1: dst texture has transparency
// #define SPRBLIT_COPYMODE <- 0/1: fast version applying only the copy mode
// #define SPRBLIT_UPATTERN <- 0/1: use drawing patterns

#include "base/mathfunc.h"
#include "engine/gfx/gfx-spr.h"
#include "engine/gfx/gfx.h"
#include "sys/sys-intrin.h"

#if !defined(SPRBLIT_FLIPPEDX)
#define SPRBLIT_FLIPPEDX 0
#endif
#if !defined(SPRBLIT_SRC_MASK)
#define SPRBLIT_SRC_MASK 0
#endif
#if !defined(SPRBLIT_COPYMODE)
#define SPRBLIT_COPYMODE 0
#endif
#if !defined(SPRBLIT_DST_MASK)
#define SPRBLIT_DST_MASK 0
#endif

// #include <assert.h>
// static_assert(SPRBLIT_FLIPPEDX <= 1, "SPRBLIT_FLIPPEDX INVALID");
// static_assert(SPRBLIT_SRC_MASK <= 1, "SPRBLIT_SRC_MASK INVALID");
// static_assert(SPRBLIT_DST_MASK <= 1, "SPRBLIT_DST_MASK INVALID");

// define reading function of source words
#if defined(SPRBLIT_FLIPPEDX) && SPRBLIT_FLIPPEDX
#define SPRBLIT_GET_WORD(ADDR) brev_u32(bswap_u32(ADDR)) // mirror bit order
#else
#define SPRBLIT_GET_WORD(ADDR) bswap_u32(ADDR)
#endif

// choose a pixel blit function depending on if destination contains transparency info
// ---
// DP: pointer to destination pixel word (black/white)
// DM: pointer to destination mask word (opaque/transparent)
// SP: assembled pixel word from source to blit to destination (black/white)
// SM: assembled mask word from source to blit to destination (opaque/transparent) - boundary clipping already applied
// PT: drawing pattern bits in screen space
// MD: blitting logic enum
#if defined(SPRBLIT_DST_MASK) && SPRBLIT_DST_MASK
#if defined(SPRBLIT_COPYMODE) && SPRBLIT_COPYMODE // force simplest blitting mode
#error blit mode not implemented
#define SPRBLIT_BLITFUNC(DP, DM, SP, SM, PT, MD) spr_blit_pm_copy(DP, DM, SP, SM, PT)
#else
#define SPRBLIT_BLITFUNC(DP, DM, SP, SM, PT, MD) spr_blit_pm(DP, DM, SP, SM, PT, MD)
#endif
#else
#if defined(SPRBLIT_COPYMODE) && SPRBLIT_COPYMODE // force simplest blitting mode
#define SPRBLIT_BLITFUNC(DP, DM, SP, SM, PT, MD) spr_blit_p_copy(DP, SP, (SM) & (PT))
#else
#define SPRBLIT_BLITFUNC(DP, DM, SP, SM, PT, MD) spr_blit_p(DP, SP, SM, PT, MD)
#endif
#endif

void
SPRBLIT_FUNCNAME(struct gfx_ctx ctx, struct tex_rec src, i32 px, i32 py, i32 flip, i32 mode)
{
	i32 x1 = max_i32(ctx.clip_x1, px);
	i32 y1 = max_i32(ctx.clip_y1, py);
	i32 x2 = min_i32(ctx.clip_x2, px + src.r.w - 1);
	i32 y2 = min_i32(ctx.clip_y2, py + src.r.h - 1);
	if(x2 < x1 || y2 < y1)
		return; // not visible

#if defined(SPRBLIT_FLIPPEDX) && SPRBLIT_FLIPPEDX
	i32 a_x = src.r.x + px + src.r.w - 1; // cached offset value
	i32 shl = 31 & (u32)(-a_x - 1);       // left shift value for word assembly
#else
	i32 a_x = src.r.x - px;     // cached offset value
	i32 shl = 31 & (u32)(+a_x); // left shift value for word assembly
#endif
	i32 shr        = 32 - shl; // right shift value for word assembly
	u32 c_l        = bswap_u32(0xFFFFFFFF >> (31 & (x1)));
	u32 c_r        = bswap_u32(0xFFFFFFFF << (31 - (x2 & 31))); // clipping mask right, << (31 & (u32)(-x2 - 1))
	i32 s_y        = flip & SPR_FLIP_Y ? -1 : +1;
	i32 a_y        = 0 < s_y ? src.r.y - py : src.r.y + py + src.r.h - 1;
	i32 dw1        = x1 >> 5;
	i32 dw2        = x2 >> 5;
	struct tex t_d = ctx.dst;
	struct tex t_s = src.t;
	dbg_assert(t_d.px);
	dbg_assert(t_s.px);

	// for every affected row of target texture
	for(i32 y_d = y1; y_d <= y2; y_d++) {
		i32 y_s = s_y * y_d + a_y; // row index in source texture
		u32 pat = ctx.pat.p[y_d & 7];

		// for every affected word in this row of the target texture
		// set clipping word to "non clipping" after the first word which has to be left clipped
		i32 d_w = dw1;
		u32 c_m = dw1 == dw2 ? c_l & c_r : c_l; // clipping word (first dst word is left clipped)

		while(1) {
			// calculate the 2 source words to pull pixel data from
#if defined(SPRBLIT_FLIPPEDX) && SPRBLIT_FLIPPEDX
			i32 sx1 = min_i32(a_x - ((d_w << 5) + 0), t_s.w - 1); // right most pixel needed from this row of source texture
			i32 sx2 = max_i32(a_x - ((d_w << 5) + 31), 0);        // left most pixel needed from this row of source texture
#else
			i32 sx1 = max_i32(a_x + ((d_w << 5) + 0), 0);          // left most pixel needed from this row of source texture
			i32 sx2 = min_i32(a_x + ((d_w << 5) + 31), t_s.w - 1); // right most pixel needed from this row of source texture
#endif
			dbg_assert(0 <= sx2 && sx2 < t_s.w);
			dbg_assert(0 <= sx1 && sx1 < t_s.w);

			// indices of left and right source words
			i32 si1 = ((sx1 >> 5) << SPRBLIT_SRC_MASK) + y_s * t_s.wword;
			i32 si2 = ((sx2 >> 5) << SPRBLIT_SRC_MASK) + y_s * t_s.wword;
#if SPRBLIT_SRC_MASK
			// assemble transparency word to blit out of 2 source words
			u32 sm1 = SPRBLIT_GET_WORD(t_s.px[si1 + 1]);
			u32 sm2 = SPRBLIT_GET_WORD(t_s.px[si2 + 1]);
			u32 smm = bswap_u32((u32)((u64)sm1 << shl) | (u32)((u64)sm2 >> shr));

			// skip empty transparent words
			if(!smm)
				goto BLITEND;

			smm &= c_m; // clipping
#else
			// opaque sprite, just use clipping bits for transparency
			u32 smm = c_m;
#endif
			{
				// assemble pixel color word to blit out of 2 source words
				u32 sp1 = SPRBLIT_GET_WORD(t_s.px[si1 + 0]);
				u32 sp2 = SPRBLIT_GET_WORD(t_s.px[si2 + 0]);
				u32 spp = bswap_u32((u32)((u64)sp1 << shl) | (u32)((u64)sp2 >> shr));

				u32 *dpp = &t_d.px[(d_w << SPRBLIT_DST_MASK) + y_d * t_d.wword];
#if SPRBLIT_DST_MASK
				u32 *dmm = dpp + 1;
				SPRBLIT_BLITFUNC(dpp, dmm, spp, smm, pat, mode);
#else
				SPRBLIT_BLITFUNC(dpp, 0, spp, smm, pat, mode);
#endif
			}

#if SPRBLIT_SRC_MASK
		BLITEND:;
#endif

			if(d_w == dw2)
				break;

			d_w++;
			c_m = (d_w == dw2 ? c_r : 0xFFFFFFFF);
		}
	}
}
#undef SPRBLIT_FUNCNAME
#undef SPRBLIT_FLIPPEDX
#undef SPRBLIT_SRC_MASK
#undef SPRBLIT_DST_MASK
#undef SPRBLIT_GET_WORD
#undef SPRBLIT_BLITFUNC
#undef SPRBLIT_COPYMODE
