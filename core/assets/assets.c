#include "assets.h"
#include "assets/asset-db.h"
#include "dbg.h"
#include "fnt/fnt.h"
#include "audio/audio.h"
#include "gfx/gfx.h"
#include "mem-arena.h"
#include "sys-log.h"
#include "str.h"
#include "sys-types.h"

void *asset_allocf(void *ctx, usize s);

void
assets_init(void *mem, usize size)
{
	log_info("Assets", "init");
	marena_init(&ASSETS.marena, mem, size);
	ASSETS.alloc   = (struct alloc){asset_allocf, (void *)&ASSETS};
	ASSETS.display = tex_frame_buffer();
}

void *
asset_allocf(void *ctx, usize s)
{
	struct assets *assets = (struct assets *)ctx;
	void *mem             = marena_alloc(&assets->marena, s);
	dbg_check_mem(mem != NULL, "Assets");
	return mem;

error: {
	usize left  = marena_size_rem(&ASSETS.marena);
	usize total = assets->marena.buf_size;
	usize used  = total - left;
	log_error("Assets", "Ran out of asset mem! requested: %u kb", (uint)s / 1024);
	log_error("Assets", "MEM: used: %u kb left: %u kb total: %u kb", (uint)used / 1024, (uint)left / 1024, (uint)total / 1024);
	assert(0);
}
}

struct tex
asset_tex(i32 id)
{
	struct tex res = asset_db_tex_get_by_id(&ASSETS.db, id);
	return res;
}

i32
asset_tex_get_id(str8 path)
{
	i32 res = asset_db_tex_get_id(&ASSETS.db, (struct asset_handle){
												  .path_hash = hash_string(path),
												  .type      = ASSET_TYPE_TEXTURE,
											  });
	return res;
}

i32
asset_tex_load(str8 path, struct tex *tex)
{
	i32 res      = 0;
	struct tex t = tex_load(path, ASSETS.alloc);

	if(t.px == NULL) {
		log_warn("Assets", "Lod failed %s", path.str);
		return -1;
	}

	log_info("Assets", "Load tex %s", path.str);
	res = asset_db_tex_push(&ASSETS.db, path, t);
	if(tex) *tex = t;
	return res;
}

struct fnt
asset_fnt(i32 id)
{
	struct fnt res = asset_db_fnt_get_by_id(&ASSETS.db, id);
	return res;
}

i32
asset_fnt_get_id(str8 path)
{
	i32 res = asset_db_fnt_get_id(&ASSETS.db, (struct asset_handle){
												  .path_hash = hash_string(path),
												  .type      = ASSET_TYPE_FONT,
											  });
	return res;
}

i32
asset_fnt_load(str8 path, struct fnt *fnt)
{
	sys_printf("Assets fnt load %s", path.str);
	i32 res = 0;

	usize size = MKILOBYTE(200);
	void *mem  = ASSETS.alloc.allocf(ASSETS.alloc.ctx, size);
	mclr(mem, size);
	struct marena marena = {0};
	struct alloc alloc   = {0};
	marena_init(&marena, mem, size);
	alloc = marena_allocator(&marena);

	usize size_scratch = MKILOBYTE(200);
	void *mem_scratch  = ASSETS.alloc.allocf(ASSETS.alloc.ctx, size_scratch);
	mclr(mem_scratch, size_scratch);
	struct marena marena_scratch = {0};
	struct alloc scratch         = {0};
	marena_init(&marena_scratch, mem_scratch, size_scratch);
	scratch = marena_allocator(&marena_scratch);

	struct fnt f = fnt_load(path, alloc, scratch);
	if(f.t.px == NULL) {
		log_warn("Assets", "Load failed %s", path.str);
	}
	res = asset_db_fnt_push(&ASSETS.db, path, f);
	log_info("Assets", "Load fnt %s", path.str);
	if(fnt) *fnt = f;

	mclr(marena.p, size_scratch);
	marena_reset_to(&ASSETS.marena, marena.p);
	sys_printf(FILE_AND_LINE);
	return res;
}

struct snd
asset_snd(i32 id)
{
	struct snd res = asset_db_snd_get_by_id(&ASSETS.db, id);
	return res;
}

i32
asset_snd_load(str8 path, struct snd *snd)
{

	i32 res      = 0;
	struct snd s = snd_load(path, ASSETS.alloc);
	if(s.len == 0) {
		log_warn("Assets", "Load failed %s", path.str);
	}
	log_info("Assets", "Load snd %s", path.str);
	res = asset_db_snd_push(&ASSETS.db, path, s);
	if(snd) *snd = s;
	return res;
}

i32
asset_snd_get_id(str8 path)
{
	i32 res = asset_db_snd_get_id(&ASSETS.db, (struct asset_handle){
												  .path_hash = hash_string(path),
												  .type      = ASSET_TYPE_SOUND,
											  });
	return res;
}

struct tex_rec
asset_tex_rec(i32 id, i32 x, i32 y, i32 w, i32 h)
{
	struct tex_rec result = {0};

	result.t   = asset_db_tex_get_by_id(&ASSETS.db, id);
	result.r.x = x;
	result.r.y = y;
	result.r.w = w;
	result.r.h = h;

	return result;
}

enum asset_type
asset_path_get_type(str8 path)
{
	str8 tex_ext = str8_lit(".tex");
	str8 snd_ext = str8_lit(".snd");
	str8 fnt_ext = str8_lit(".fnt");
	str8 bet_ext = str8_lit(".bet");

	if(str8_ends_with(path, tex_ext, 0)) {
		return ASSET_TYPE_TEXTURE;
	} else if(str8_ends_with(path, snd_ext, 0)) {
		return ASSET_TYPE_SOUND;
	} else if(str8_ends_with(path, fnt_ext, 0)) {
		return ASSET_TYPE_FONT;
	} else if(str8_ends_with(path, bet_ext, 0)) {
		return ASSET_TYPE_BET;
	}

	return 0;
}
