#pragma once

#include "sys-types.h"
#include "assets/assets-db.h"

#include "gfx.h"
#include "mem.h"
#include "mem-arena.h"

#define FILE_PATH_TEX "assets/"

enum {
	TEX_ID_NONE,
	TEX_ID_DISPLAY,

	// Debug
	TEX_ID_LOADING,

	NUM_TEX_ID,
	NUM_TEX_ID_MAX = 512
};

struct asset_tex {
	struct tex tex;
	str8 file_name;
};

struct assets {
	char assets_paths[NUM_TEX_ID_MAX][30];
	struct asset_tex tex[NUM_TEX_ID_MAX];

	i32 next_tex_id;
	struct assets_db assets_db;

	struct marena marena;
	struct alloc alloc;
};

struct assets ASSETS;

struct alloc assets_allocator(struct assets *assets);

void assets_init(void *mem, usize size);
struct tex asset_tex(i32 id);
i32 asset_tex_load(const str8 file_name, struct tex *tex);
i32 asset_tex_load_id(i32 id, str8 file_name, struct tex *tex);
i32 asset_tex_get_id(str8 file_name);

i32 asset_tex_put(struct tex tex);
struct tex asset_tex_put_id(i32 id, struct tex tex);

struct tex_rec asset_tex_rec(i32 id, i32 x, i32 y, i32 w, i32 h);
