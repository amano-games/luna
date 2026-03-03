#include "gfx-spr.h"

#define SPRBLIT_FUNCNAME gfx_spr_d_s
#define SPRBLIT_SRC_MASK 0
#define SPRBLIT_DST_MASK 0
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

void
gfx_spr_cpy(struct gfx_ctx ctx, struct tex_rec src, i32 px, i32 py, i32 flip)
{
	dbg_assert(src.t.fmt == TEX_FMT_MASK);
	dbg_assert(ctx.dst.fmt == TEX_FMT_OPAQUE);
	if(flip & SPR_FLIP_X) {
		gfx_spr_sm_fx_copy(ctx, src, px, py, flip, 0);
	} else {
		gfx_spr_sm_copy(ctx, src, px, py, flip, 0);
	}
}

void
gfx_spr(struct gfx_ctx ctx, struct tex_rec src, i32 px, i32 py, enum spr_flip flip, enum spr_mode mode)
{
	if(!src.t.px)
		return;
	if(ctx.dst.fmt == TEX_FMT_OPAQUE) {
		if(src.t.fmt == TEX_FMT_OPAQUE) {
			gfx_spr_d_s(ctx, src, px, py, flip, mode);
		} else {
			if(flip & SPR_FLIP_X) {
				gfx_spr_sm_fx(ctx, src, px, py, flip, mode);
			} else {
				gfx_spr_sm(ctx, src, px, py, flip, mode);
			}
		}
	} else {
		if(flip & SPR_FLIP_X) {
			gfx_spr_dm_sm_fx(ctx, src, px, py, flip, mode);
		} else {
			gfx_spr_dm_sm(ctx, src, px, py, flip, mode);
		}
	}
}
