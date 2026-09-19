#pragma once

#include "asset-defs.h"
#include "base/mem.h"
#include "base/types.h"
#include "lib/tex/tex.h"

enum asset_flag {
	ASSET_FLAG_NONE,
	ASSET_FLAG_LZ4HC,
};

b32 asset_blob_w(struct asset_blob blob, str8 out_path);
ssize asset_lz4hc(struct alloc scratch, const void *raw, ssize raw_size, void **out, u32 *flags, u32 flag_lz4);
b32 tex_to_blob(struct alloc scratch, struct alloc alloc, struct tex t, struct asset_blob *out, enum asset_flag flag);
