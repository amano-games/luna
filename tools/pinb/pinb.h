#pragma once

#include "mem.h"
#include "sys-types.h"
#define PINBALL_EXT "lunpinb"

i32 handle_pinball_table(str8 in_path, str8 out_path, struct alloc scratch);
