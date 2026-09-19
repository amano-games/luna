#pragma once

#include "engine/assets/asset-db.h"
#include "base/mem.h"
#include "tools/asset/asset-defs.h"

#define ATLAS_EXT "atlas"

b32 atlas_to_blob(struct alloc alloc, struct tex_atlas atlas, struct asset_blob *out);
struct tex_atlas atlas_from_mem(void *data, ssize size);
