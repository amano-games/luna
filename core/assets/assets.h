#pragma once

#include "audio/audio.h"
#include "sys-types.h"
#include "assets/asset-db.h"

#include "gfx/gfx.h"
#include "mem.h"
#include "mem-arena.h"

struct assets {
	struct asset_db db;
	struct tex display;

	struct marena scratch_marena;
	struct alloc scratch_alloc;

	struct marena marena;
	struct alloc alloc;
};

static struct assets ASSETS;
struct alloc assets_allocator(struct assets *assets);

void assets_init(void *mem, usize size);

struct tex asset_tex(i32 id);
i32 asset_tex_load(str8 path, struct tex *tex);
i32 asset_tex_get_id(str8 path);

struct fnt asset_fnt(i32 id);
i32 asset_fnt_load(str8 path, struct fnt *fnt);
i32 asset_fnt_get_id(str8 path);

struct snd asset_snd(i32 id);
i32 asset_snd_load(str8 path, struct snd *snd);
i32 asset_snd_get_id(str8 path);

struct tex_rec asset_tex_rec(i32 id, i32 x, i32 y, i32 w, i32 h);
struct tex_patch asset_tex_patch(i32 id, i32 x, i32 y, i32 w, i32 h, i32 ml, i32 mr, i32 mt, i32 mb);
enum asset_type asset_path_get_type(str8 path);
str8 asset_path_to_full_path(struct str8 path);
