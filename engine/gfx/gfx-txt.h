#pragma once

#include "base/str.h"
#include "lib/fnt/fnt.h"
#include "engine/gfx/gfx.h"

enum fnt_align {
	FNT_ALIGN_H_START   = 1 << 0,
	FNT_ALIGN_H_CENTER  = 1 << 1,
	FNT_ALIGN_H_END     = 1 << 2,
	FNT_ALIGN_H_JUSTIFY = 1 << 3,

	FNT_ALIGN_V_START  = 1 << 4,
	FNT_ALIGN_V_CENTER = 1 << 5,
	FNT_ALIGN_V_END    = 1 << 6,
};

void fnt_draw_str(struct gfx_ctx ctx, struct fnt fnt, str8 str, i32 x, i32 y, i32 tracking, i32 leading, i32 mode);
rec_i32 fnt_draw_str_pivot(struct gfx_ctx ctx, struct fnt fnt, str8 str, i32 tracking, i32 leading, i32 x, i32 y, v2 pivot, enum spr_mode mode);
rec_i32 fnt_draw_str_block(struct gfx_ctx ctx, struct fnt fnt, struct str8_list lines, rec_i32 layout, i32 tracking, i32 leading, enum spr_mode mode, u32 align_flags);
