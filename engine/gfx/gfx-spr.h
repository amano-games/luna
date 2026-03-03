#pragma once

#include "gfx.h"

void gfx_spr_cpy(struct gfx_ctx ctx, struct tex_rec src, i32 px, i32 py, i32 flip);
void gfx_spr(struct gfx_ctx ctx, struct tex_rec src, i32 px, i32 py, enum spr_flip flip, enum spr_mode mode);

// branchless?
static void
spr_blit_p_res(u32 dp, u32 dm, u32 sp, u32 sm, u32 pt, enum spr_mode mode, u32 *out_dp, u32 *out_dm)
{
	u32 zm     = sm & pt;
	u32 dp_set = 0;
	u32 dp_clr = 0;
	u32 dm_set = 0;
	u32 dm_clr = 0;

	switch(mode) {
	case SPR_MODE_COPY:
		dm_set = zm;
		dp_clr = zm;
		dp_set = sp & zm;
		break;
	case SPR_MODE_INV:
		dm_set = zm;
		dp_clr = zm;
		dp_set = ~sp & zm;
		break;
	case SPR_MODE_BLACK:
		dm_set = zm;
		dp_clr = zm;
		break;
	case SPR_MODE_WHITE:
		dm_set = zm;
		dp_set = zm;
		break;
	case SPR_MODE_BLACK_ONLY:
		dm_set = zm & ~sp;
		dp_clr = zm & ~sp;
		break;
	case SPR_MODE_WHITE_ONLY:
		dm_set = zm & sp;
		dp_set = zm & sp;
		break;
	case SPR_MODE_XOR:
		dp_clr = zm & (~sp ^ dp);
		dp_set = zm & (sp ^ dp);
		break;
	case SPR_MODE_NXOR:
		dp_clr = zm & (sp ^ dp);
		dp_set = zm & (~sp ^ dp);
		break;
	}

	*out_dp = (dp & ~dp_clr) | dp_set;
	*out_dm = (dm & ~dm_clr) | dm_set;
}

static void
spr_blit_p(u32 *dp, u32 sp, u32 sm, u32 pt, i32 mode)
{
	u32 dp_res;
	u32 dm_res;
	spr_blit_p_res(*dp, 0, sp, sm, pt, mode, &dp_res, &dm_res);
	*dp = dp_res;
}

static void
spr_blit_pm(u32 *dp, u32 *dm, u32 sp, u32 sm, u32 pt, i32 mode)
{
	u32 dp_res;
	u32 dm_res;
	spr_blit_p_res(*dp, *dm, sp, sm, pt, mode, &dp_res, &dm_res);
	*dp = dp_res;
	*dm = dm_res;
}

static inline void
spr_blit_p_copy(u32 *dp, u32 sp, u32 sm)
{
	*dp = (*dp & ~sm) | (sp & sm);
}
