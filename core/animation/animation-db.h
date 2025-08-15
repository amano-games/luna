#pragma once

#include "assets/asset-db.h"
#include "sys-types.h"
#include "serialize/serialize.h"

struct ani_db {
	struct ani_db_asset *assets;
	usize clip_count;
	usize bank_count;
};

struct ani_db_asset {
	str8 path;
	struct asset_tex_info info;
	struct animation_clip *clips;
};

void ani_db_write(struct ser_writer *w, struct ani_db db);
struct ani_db ani_db_read(struct ser_reader *r, struct alloc alloc);
