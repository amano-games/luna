#pragma once

#include "base/mem.h"
#include "base/types.h"
#include "engine/gfx/gfx-defs.h"
#include "lib/tex/tex.h"

b32 sys_img_write(struct tex tex, str8 path, struct gfx_col_pallete pallete, struct alloc scratch);
