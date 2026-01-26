#pragma once

#include "base/mem.h"
#include "base/types.h"
#include "tools/asset/asset-defs.h"

b32 png_to_tex_blob(str8 in_path, struct alloc scratch, struct alloc alloc, struct asset_blob *out);
