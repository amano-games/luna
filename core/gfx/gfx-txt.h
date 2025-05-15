#pragma once

#include "assets/fnt.h"
#include "gfx.h"

void fnt_draw_str(struct gfx_ctx ctx, struct fnt fnt, i32 x, i32 y, str8 str, i32 tracking, i32 leading, i32 mode);
rec_i32 fnt_draw_str_pivot(struct gfx_ctx ctx, struct fnt fnt, str8 str, i32 x, i32 y, v2 pivot, enum spr_mode mode);
