#pragma once

#include "asset-defs.h"
#include "base/mem.h"
#include "base/types.h"
#include "lib/tex/tex.h"

b32 asset_blob_w(struct asset_blob blob, str8 out_path);
b32 tex_to_blob(struct alloc scratch, struct alloc alloc, struct tex t, struct asset_blob *out);
