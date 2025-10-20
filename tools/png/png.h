#pragma once

#include "base/mem.h"
#include "base/types.h"

b32 png_to_tex(const str8 in_path, const str8 out_path, struct alloc scratch);
