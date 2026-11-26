#pragma once

#include "base/mem.h"
#include "base/types.h"

#define LOC_TSV_EXT      ".loc.tsv"
#define LOC_EXT          "loc"
#define LOC_VERSION      1
#define LOC_LANG_ID_SIZE 8
#define LOC_MAGIC \
	(((u32)'L') << 0 | ((u32)'L') << 8 | \
		((u32)'O') << 16 | ((u32)'C') << 24)

struct loc_header {
	u32 magic;
	u32 version;
	u32 lang_count;
	u32 key_count;
	u64 keys_hash;
};

struct loc_lang_entry {
	char id[LOC_LANG_ID_SIZE];
	u32 offset;
	u32 size;
};

i32 loc_gen_handle(str8 in_path, str8 out_path, struct alloc scratch);
