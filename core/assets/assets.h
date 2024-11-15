#pragma once

#include "audio/audio.h"
#include "sys-types.h"
#include "assets/asset-db.h"

#include "gfx/gfx.h"
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

enum {
	SFX_NONE,
	SFX_CLICK,
	SFX_FLIPPER,
	SFX_IMPACT,
	SFX_SWITCH,
	SFX_TOGGLE,
	NUM_SFX_ID,
	NUM_SFX_ID_MAX = 100,
};

struct asset_tex {
	struct tex tex;
	str8 file_name;
};

struct asset_snd {
	struct snd snd;
	// Add path handle (hash)
};

struct assets {
	struct asset_tex tex[NUM_TEX_ID_MAX];
	struct asset_snd snd[NUM_SFX_ID_MAX];

	i32 next_tex_id;
	i32 next_snd_id;
	struct asset_db db;

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

struct snd asset_snd(i32 id);
i32 asset_snd_load(const str8 file_name, struct snd *snd);
i32 asset_snd_load_id(i32 id, str8 file_name, struct snd *snd);

struct tex_rec asset_tex_rec(i32 id, i32 x, i32 y, i32 w, i32 h);

enum asset_type asset_path_get_type(str8 path);
struct fnt assets_fnt_load(str8 path, struct alloc alloc, struct alloc scratch);
