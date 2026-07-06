#pragma once

#include "base/types.h"
#include "engine/assets/asset-db.h"

#include "engine/gfx/gfx.h"
#include "base/mem.h"
#include "base/marena.h"

struct assets {
	struct asset_db db;
	struct tex display;

	struct marena marena;
	struct alloc alloc;
};

extern struct assets ASSETS;
struct alloc assets_allocator(struct assets *assets);

void assets_ini(struct alloc alloc, usize size);

struct tex asset_tex(i32 id);
i32 asset_tex_load(struct alloc scratch, str8 path, struct tex *tex);
i32 asset_tex_get_id(str8 path);

struct fnt asset_fnt(i32 id);
i32 asset_fnt_load(struct alloc scratch, str8 path, struct fnt *fnt);
i32 asset_fnt_get_id(str8 path);

struct snd asset_snd(i32 id);
i32 asset_snd_load(struct alloc scratch, str8 path, struct snd *snd);
i32 asset_snd_get_id(str8 path);

struct bet asset_bet(i32 id);
i32 asset_bet_load(struct alloc scratch, str8 path, struct bet *bet);
i32 asset_bet_get_id(str8 path);

struct tex_rec asset_tex_rec(i32 id, i32 x, i32 y, i32 w, i32 h);
struct tex_patch asset_tex_patch(i32 id, i32 x, i32 y, i32 w, i32 h, i32 ml, i32 mr, i32 mt, i32 mb);
enum asset_type asset_path_get_type(str8 path);
str8 asset_path_to_full_path(struct alloc scratch, struct str8 path);
