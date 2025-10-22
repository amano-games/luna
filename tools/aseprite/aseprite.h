#pragma once

#include "external/cute_aseprite.h"

#include "base/mem.h"
#include "base/types.h"

b32 aseprite_to_assets(const str8 in_path, const str8 out_path, struct alloc scratch);
b32 aseprite_to_tex(const ase_t *ase, const str8 in_path, const str8 out_path, struct alloc scratch);
b32 aseprite_to_ani(const ase_t *ase, const str8 in_path, const str8 out_path, struct alloc scratch);
