#pragma once

#include "gfx.h"

void
gfx_spr(struct gfx_ctx ctx, struct tex_rec src, i32 px, i32 py, enum spr_flip flip, enum spr_mode mode);
void gfx_patch(struct gfx_ctx ctx, struct tex_patch patch, i32 px, i32 py, i32 w, i32 h, enum spr_flip flip, enum spr_mode mode);
