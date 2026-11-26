#pragma once

#include "base/ht.h"
#include "base/mem.h"
#include "base/str.h"
#include "base/types.h"

#define LOC_EXT          "loc"
#define LOC_VERSION      1
#define LOC_LANG_ID_SIZE 8
#define LOC_LANGS_MAX    8
#define LOC_MAGIC \
	(((u32)'L') << 0 | ((u32)'L') << 8 | \
		((u32)'O') << 16 | ((u32)'C') << 24)

struct loc_header {
	u32 magic;
	u32 version;
	u32 lang_count;
	u32 key_count;
	u32 keys_offset;
	u32 keys_size;
};

struct loc_lang_entry {
	char id[LOC_LANG_ID_SIZE];
	u32 offset;
	u32 size;
};

struct loc_pool {
	u32 *offsets;
	u8 *data;
};

struct loc {
	u32 key_count;
	u32 lang_count;
	i32 lang;
	struct loc_lang_entry langs[LOC_LANGS_MAX];
	struct loc_pool keys;
	struct loc_pool strs;
	struct ht_u32 ht;
};

b32 loc_load_from_mem(struct loc *loc, struct alloc alloc, void *data, usize size);
b32 loc_lang_set_from_mem(struct loc *loc, struct alloc alloc, void *data, usize size, i32 lang);
i32 loc_lang_idx(struct loc *loc, str8 id);
str8 loc_lang_id(struct loc *loc, i32 lang);
str8 loc_get(struct loc *loc, str8 key);
