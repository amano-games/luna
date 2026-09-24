#include "gfx-spr.h"
#include "base/dbg.h"
#include "base/mathfunc.h"

#define SPRBLIT_FUNCNAME gfx_spr_d_s
#define SPRBLIT_SRC_MASK 0
#define SPRBLIT_DST_MASK 0
#define SPRBLIT_FLIPPEDX 0
#include "gfx-spr-func.h"
#undef SPRBLIT_FUNCNAME
#undef SPRBLIT_SRC_MASK
#undef SPRBLIT_DST_MASK
#undef SPRBLIT_FLIPPEDX

#define SPRBLIT_FUNCNAME gfx_spr_dm_s
#define SPRBLIT_SRC_MASK 0
#define SPRBLIT_DST_MASK 1
#define SPRBLIT_FLIPPEDX 0
#include "gfx-spr-func.h"
#undef SPRBLIT_FUNCNAME
#undef SPRBLIT_SRC_MASK
#undef SPRBLIT_DST_MASK
#undef SPRBLIT_FLIPPEDX

#define SPRBLIT_FUNCNAME gfx_spr_dm_sm
#define SPRBLIT_SRC_MASK 1
#define SPRBLIT_DST_MASK 1
#define SPRBLIT_FLIPPEDX 0
#include "gfx-spr-func.h"
#undef SPRBLIT_FUNCNAME
#undef SPRBLIT_SRC_MASK
#undef SPRBLIT_DST_MASK
#undef SPRBLIT_FLIPPEDX

#define SPRBLIT_FUNCNAME gfx_spr_sm
#define SPRBLIT_SRC_MASK 1
#define SPRBLIT_DST_MASK 0
#define SPRBLIT_FLIPPEDX 0
#include "gfx-spr-func.h"
#undef SPRBLIT_FUNCNAME
#undef SPRBLIT_SRC_MASK
#undef SPRBLIT_DST_MASK
#undef SPRBLIT_FLIPPEDX

#define SPRBLIT_FUNCNAME gfx_spr_dm_s_fx
#define SPRBLIT_SRC_MASK 0
#define SPRBLIT_DST_MASK 1
#define SPRBLIT_FLIPPEDX 1
#include "gfx-spr-func.h"
#undef SPRBLIT_FUNCNAME
#undef SPRBLIT_SRC_MASK
#undef SPRBLIT_DST_MASK
#undef SPRBLIT_FLIPPEDX

#define SPRBLIT_FUNCNAME gfx_spr_dm_sm_fx
#define SPRBLIT_SRC_MASK 1
#define SPRBLIT_DST_MASK 1
#define SPRBLIT_FLIPPEDX 1
#include "gfx-spr-func.h"
#undef SPRBLIT_FUNCNAME
#undef SPRBLIT_SRC_MASK
#undef SPRBLIT_DST_MASK
#undef SPRBLIT_FLIPPEDX

#define SPRBLIT_FUNCNAME gfx_spr_sm_fx
#define SPRBLIT_SRC_MASK 1
#define SPRBLIT_DST_MASK 0
#define SPRBLIT_FLIPPEDX 1
#include "gfx-spr-func.h"
#undef SPRBLIT_FUNCNAME
#undef SPRBLIT_SRC_MASK
#undef SPRBLIT_DST_MASK
#undef SPRBLIT_FLIPPEDX

#define SPRBLIT_FUNCNAME gfx_spr_sm_fx_copy
#define SPRBLIT_SRC_MASK 1
#define SPRBLIT_DST_MASK 0
#define SPRBLIT_FLIPPEDX 1
#define SPRBLIT_COPYMODE 1
#include "gfx-spr-func.h"
#undef SPRBLIT_FUNCNAME
#undef SPRBLIT_SRC_MASK
#undef SPRBLIT_DST_MASK
#undef SPRBLIT_FLIPPEDX
#undef SPRBLIT_COPYMODE

#define SPRBLIT_FUNCNAME gfx_spr_sm_copy
#define SPRBLIT_SRC_MASK 1
#define SPRBLIT_DST_MASK 0
#define SPRBLIT_FLIPPEDX 0
#define SPRBLIT_COPYMODE 1
#include "gfx-spr-func.h"
#undef SPRBLIT_FUNCNAME
#undef SPRBLIT_SRC_MASK
#undef SPRBLIT_DST_MASK
#undef SPRBLIT_FLIPPEDX
#undef SPRBLIT_COPYMODE

#define SPRBLIT_FUNCNAME gfx_spr_d_s_copy
#define SPRBLIT_SRC_MASK 0
#define SPRBLIT_DST_MASK 0
#define SPRBLIT_FLIPPEDX 0
#define SPRBLIT_COPYMODE 1
#include "gfx-spr-func.h"
#undef SPRBLIT_FUNCNAME
#undef SPRBLIT_SRC_MASK
#undef SPRBLIT_DST_MASK
#undef SPRBLIT_FLIPPEDX
#undef SPRBLIT_COPYMODE

#define SPRBLIT_FUNCNAME gfx_spr_d_s_fx_copy
#define SPRBLIT_SRC_MASK 0
#define SPRBLIT_DST_MASK 0
#define SPRBLIT_FLIPPEDX 1
#define SPRBLIT_COPYMODE 1
#include "gfx-spr-func.h"
#undef SPRBLIT_FUNCNAME
#undef SPRBLIT_SRC_MASK
#undef SPRBLIT_DST_MASK
#undef SPRBLIT_FLIPPEDX
#undef SPRBLIT_COPYMODE

static void
gfx_spr_8b(struct gfx_ctx ctx, struct tex_rec src, i32 px, i32 py, i32 flip, enum spr_mode mode)
{
	if(!src.t.pxu8 || !ctx.dst.pxu8) return;
	if(src.t.fmt != TEX_FMT_8B_INDEX && src.t.fmt != TEX_FMT_1B_OPAQUE && src.t.fmt != TEX_FMT_1B_MASK) {
		dbg_assert(0);
		return;
	}

	i32 x1 = max_i32(max_i32(ctx.clip_x1, 0), px);
	i32 y1 = max_i32(max_i32(ctx.clip_y1, 0), py);
	i32 x2 = min_i32(min_i32(ctx.clip_x2, ctx.dst.w - 1), px + src.r.w - 1);
	i32 y2 = min_i32(min_i32(ctx.clip_y2, ctx.dst.h - 1), py + src.r.h - 1);

	for(i32 y = y1; y <= y2; ++y) {
		i32 sy = src.r.y + ((flip & SPR_FLIP_Y) ? src.r.h - 1 - (y - py) : y - py);
		u8 *dp = ctx.dst.pxu8 + (ssize)y * ctx.dst.wword * sizeof(u32);

		for(i32 x = x1; x <= x2; ++x) {
			u32 pat_bit = bswap_u32(0x80000000U >> (x & 31));

			if(!(ctx.pat.p[y & 7] & pat_bit)) continue;

			i32 sx = src.r.x + ((flip & SPR_FLIP_X) ? src.r.w - 1 - (x - px) : x - px);

			if(src.t.fmt == TEX_FMT_8B_INDEX) {
				dp[x] = src.t.pxu8[(ssize)sy * src.t.wword * sizeof(u32) + sx];
			} else {
				b32 masked = src.t.fmt == TEX_FMT_1B_MASK;
				u32 bit    = bswap_u32(0x80000000U >> (sx & 31));
				u32 *sp    = src.t.px1b + (ssize)sy * src.t.wword + ((sx >> 5) << masked);
				if(masked && !(sp[1] & bit)) continue;
				u8 col = (sp[0] & bit) != 0;
				switch(mode) {
				case SPR_MODE_COPY: dp[x] = col; break;
				case SPR_MODE_INV: dp[x] = col ^ 1; break;
				case SPR_MODE_BLACK: dp[x] = 0; break;
				case SPR_MODE_WHITE: dp[x] = 1; break;
				case SPR_MODE_BLACK_ONLY:
					if(!col) dp[x] = 0;
					break;
				case SPR_MODE_WHITE_ONLY:
					if(col) dp[x] = 1;
					break;
				case SPR_MODE_XOR:
				case SPR_MODE_NXOR:
					// Only monochrome palette indices have a defined inverse.
					if(dp[x] <= 1 && col == (mode == SPR_MODE_XOR)) dp[x] ^= 1;
					break;
				}
			}
		}
	}
}

void
gfx_spr_cpy(struct gfx_ctx ctx, struct tex_rec src, i32 px, i32 py, i32 flip)
{
	if(ctx.dst.fmt == TEX_FMT_8B_INDEX) {
		gfx_spr_8b(ctx, src, px, py, flip, SPR_MODE_COPY);
	} else {
		dbg_assert(src.t.fmt != TEX_FMT_8B_INDEX);
		dbg_assert(ctx.dst.fmt == TEX_FMT_1B_OPAQUE);

		if(src.t.fmt == TEX_FMT_1B_OPAQUE) {
			if(flip & SPR_FLIP_X) {
				gfx_spr_d_s_fx_copy(ctx, src, px, py, flip, 0);
			} else {
				gfx_spr_d_s_copy(ctx, src, px, py, flip, 0);
			}
		} else {
			dbg_assert(src.t.fmt == TEX_FMT_1B_MASK);
			if(flip & SPR_FLIP_X) {
				gfx_spr_sm_fx_copy(ctx, src, px, py, flip, 0);
			} else {
				gfx_spr_sm_copy(ctx, src, px, py, flip, 0);
			}
		}
	}
}

void
gfx_spr_d_s_cpy_fast(struct gfx_ctx ctx, struct tex src, i32 px, i32 py)
{
	dbg_assert(src.fmt == TEX_FMT_1B_OPAQUE);
	dbg_assert(ctx.dst.fmt == TEX_FMT_1B_OPAQUE);
	dbg_assert(src.px1b != NULL);
	dbg_assert(ctx.dst.px1b != NULL);

	struct tex dst = ctx.dst;
	i32 x1         = max_i32(ctx.clip_x1, 0);
	i32 y1         = max_i32(ctx.clip_y1, 0);
	i32 x2         = min_i32(ctx.clip_x2, dst.w - 1);
	i32 y2         = min_i32(ctx.clip_y2, dst.h - 1);

	x1 = max_i32(x1, px);
	y1 = max_i32(y1, py);
	x2 = min_i32(x2, px + src.w - 1);
	y2 = min_i32(y2, py + src.h - 1);
	if(x2 < x1 || y2 < y1) {
		return;
	}

	i32 src_x0  = -px;
	i32 src_y0  = -py;
	i32 dw1     = x1 >> 5;
	i32 dw2     = x2 >> 5;
	i32 nwords  = dw2 - dw1 + 1;
	i32 aligned = ((x1 & 31) == 0) && ((src_x0 & 31) == 0);

	if(aligned) {
		i32 src_word = (x1 - px) >> 5;
		i32 avail    = src.wword - src_word;
		nwords       = min_i32(nwords, avail);
		nwords       = min_i32(nwords, dst.wword - dw1);
		if(src_word >= 0 && nwords > 0) {
			usize row_b = (usize)nwords * sizeof(u32);
			for(i32 y = y1; y <= y2; ++y) {
				u32 *sp = src.px1b + (src_y0 + y) * src.wword + src_word;
				u32 *dp = dst.px1b + y * dst.wword + dw1;
				mcpy(dp, sp, row_b);
			}
		}
		return;
	}

	i32 shl = src_x0 & 31;
	i32 shr = 32 - shl;
	i32 sw0 = src_x0 >> 5;
	for(i32 y = y1; y <= y2; ++y) {
		u32 *src_row = src.px1b + (src_y0 + y) * src.wword;
		u32 *dst_row = dst.px1b + y * dst.wword;
		for(i32 dw = dw1; dw <= dw2; ++dw) {
			i32 s0      = sw0 + dw;
			u32 w0      = (0 <= s0 && s0 < src.wword) ? bswap_u32(src_row[s0]) : 0;
			u32 w1      = (0 <= s0 + 1 && s0 + 1 < src.wword) ? bswap_u32(src_row[s0 + 1]) : 0;
			dst_row[dw] = bswap_u32((u32)((u64)w0 << shl) | (u32)((u64)w1 >> shr));
		}
	}
}

void
gfx_spr(struct gfx_ctx ctx, struct tex_rec src, i32 px, i32 py, enum spr_flip flip, enum spr_mode mode)
{
	if(!src.t.px1b) return;

	if(ctx.dst.fmt == TEX_FMT_8B_INDEX) {
		dbg_assert(src.t.fmt != TEX_FMT_8B_INDEX || mode == SPR_MODE_COPY);
		gfx_spr_8b(ctx, src, px, py, flip, mode);

	} else {

		dbg_assert(src.t.fmt != TEX_FMT_8B_INDEX);

		if(ctx.dst.fmt == TEX_FMT_1B_OPAQUE) {
			if(mode == SPR_MODE_COPY) {
				gfx_spr_cpy(ctx, src, px, py, flip);
			} else if(src.t.fmt == TEX_FMT_1B_OPAQUE) {
				gfx_spr_d_s(ctx, src, px, py, flip, mode);
			} else {
				if(flip & SPR_FLIP_X) {
					gfx_spr_sm_fx(ctx, src, px, py, flip, mode);
				} else {
					gfx_spr_sm(ctx, src, px, py, flip, mode);
				}
			}
		} else {
			if(src.t.fmt == TEX_FMT_1B_OPAQUE) {
				if(flip & SPR_FLIP_X) {
					gfx_spr_dm_s_fx(ctx, src, px, py, flip, mode);
				} else {
					gfx_spr_dm_s(ctx, src, px, py, flip, mode);
				}
			} else {
				if(flip & SPR_FLIP_X) {
					gfx_spr_dm_sm_fx(ctx, src, px, py, flip, mode);
				} else {
					gfx_spr_dm_sm(ctx, src, px, py, flip, mode);
				}
			}
		}
	}
}
