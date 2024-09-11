#pragma once

#include "animation/animation-db.h"
#include "sys-types.h"

#include "gfx.h"
#include "mem-arena.h"

#define FILE_PATH_TEX "assets/"

enum {
	TEX_ID_DISPLAY,

	// Debug
	TEX_ID_LOADING,

	NUM_TEX_ID,
	NUM_TEX_ID_MAX = 512
};

struct asset_tex {
	struct tex tex;
	char file[32];
};

struct assets {
	struct asset_tex tex[NUM_TEX_ID_MAX];

	int next_tex_id;
	struct animation_db animation_db;

	struct marena marena;
};

struct assets ASSETS;

void *asset_mem_alloc(usize s);

void assets_init(void *mem, usize size);
struct tex asset_tex(int id);
int asset_tex_load(const char *file_name, struct tex *tex);
int asset_tex_load_id(int id, const char *file_name, struct tex *tex);

int asset_tex_put(struct tex tex);
struct tex asset_tex_put_id(int id, struct tex tex);

struct tex_rec asset_tex_rec(int id, int x, int y, int w, int h);
