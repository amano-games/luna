#pragma once

#include "base/mem.h"
#include "base/types.h"
#include "lib/loc/loc.h"

#define LOC_TSV_EXT ".loc.tsv"

i32 loc_gen_handle(str8 in_path, str8 out_path, struct alloc scratch);
