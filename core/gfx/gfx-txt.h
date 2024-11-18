#pragma once

#include "assets/fnt.h"
#include "gfx.h"

void fnt_draw_str(struct gfx_ctx ctx, struct fnt fnt, i32 x, i32 y, str8 str, i32 tracking, i32 leading, i32 mode);
