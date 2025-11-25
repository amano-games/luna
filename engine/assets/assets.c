#include "assets.h"
#include "engine/assets/asset-db.h"
#include "base/dbg.h"
#include "lib/bet/bet-ser.h"
#include "lib/fnt/fnt.h"
#include "engine/gfx/gfx.h"
#include "base/marena.h"
#include "base/path.h"
#include "base/log.h"
#include "base/str.h"
#include "base/types.h"

void *asset_allocf(void *ctx, usize s);

void
assets_ini(struct alloc alloc, usize size)
{
	log_info("Assets", "init");
	void *mem = alloc.allocf(alloc.ctx, size);
	marena_init(&ASSETS.marena, mem, size);
	ASSETS.alloc   = (struct alloc){asset_allocf, (void *)&ASSETS};
	ASSETS.display = tex_frame_buffer();

	{
		usize scratch_size = MKILOBYTE(50);
		void *scratch_mem  = ASSETS.alloc.allocf(ASSETS.alloc.ctx, scratch_size);
		marena_init(&ASSETS.scratch_marena, scratch_mem, scratch_size);
		ASSETS.scratch_alloc = marena_allocator(&ASSETS.scratch_marena);
	}
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
	log_error("Assets", "Ran out of asset mem! requested: %$u", (uint)s);
	log_error("Assets", "MEM: used: %$u left: %$u total: %$u", (uint)used, (uint)left, (uint)total);
	return NULL;
}
}

struct tex
asset_tex(i32 id)
{
	TRACE_START(__func__);
	struct asset_tex res = asset_db_tex_get_by_id(&ASSETS.db, id);
	TRACE_END();
	return res.tex;
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
	i32 res        = 0;
	str8 full_path = asset_path_to_full_path(path);
	struct tex t   = tex_load(full_path, ASSETS.alloc);

	if(t.px == NULL) {
		log_warn("Assets", "Tex loading failed: %s", full_path.str);
		return -1;
	}

	log_info("Assets", "Tex loaded: %s", path.str);
	res = asset_db_tex_push(&ASSETS.db, path, t);
	if(tex) *tex = t;
	return res;
}

struct fnt
asset_fnt(i32 id)
{
	struct asset_fnt res = asset_db_fnt_get_by_id(&ASSETS.db, id);
	return res.fnt;
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

	str8 full_path = asset_path_to_full_path(path);
	struct fnt f   = fnt_load(full_path, alloc, scratch);
	if(f.t.px == NULL) {
		log_warn("Assets", "Load failed %s", full_path.str);
	}
	res = asset_db_fnt_push(&ASSETS.db, path, f);
	log_info("Assets", "Load fnt %s", path.str);
	if(fnt) *fnt = f;

	mclr(marena.p, size_scratch);
	marena_reset_to(&ASSETS.marena, marena.p);
	return res;
}

struct snd
asset_snd(i32 id)
{
	struct asset_snd res = asset_db_snd_get_by_id(&ASSETS.db, id);
	return res.snd;
}

i32
asset_snd_load(str8 path, struct snd *snd)
{

	i32 res        = 0;
	str8 full_path = asset_path_to_full_path(path);
	struct snd s   = snd_load(full_path, ASSETS.alloc);
	if(s.len == 0) {
		log_warn("Assets", "Load failed %s", full_path.str);
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

struct bet
asset_bet(i32 id)
{
	struct asset_bet res = asset_db_bet_get_by_id(&ASSETS.db, id);
	return res.bet;
}

i32
asset_bet_load(str8 path, struct bet *bet)
{
	marena_reset(&ASSETS.scratch_marena);
	struct alloc scratch = ASSETS.scratch_alloc;
	struct alloc alloc   = ASSETS.alloc;
	i32 res              = 0;
	str8 full_path       = asset_path_to_full_path(path);
	struct bet b         = bet_load(full_path, alloc, scratch);
	if(b.nodes == NULL) {
		log_warn("Assets", "Bet loading failed: %s", full_path.str);
		return -1;
	}
	log_info("Assets", "Bet loaded: %s", path.str);
	res = asset_db_bet_push(&ASSETS.db, path, b);
	if(bet) *bet = b;
	return res;
}

i32
asset_bet_get_id(str8 path)
{
	i32 res = asset_db_bet_get_id(&ASSETS.db, (struct asset_handle){
												  .path_hash = hash_string(path),
												  .type      = ASSET_TYPE_BET,
											  });
	return res;
}

struct tex_rec
asset_tex_rec(i32 id, i32 x, i32 y, i32 w, i32 h)
{
	TRACE_START(__func__);

	struct tex_rec res = {0};
	res.t              = asset_db_tex_get_by_id(&ASSETS.db, id).tex;
	res.r.x            = x;
	res.r.y            = y;
	res.r.w            = w;
	res.r.h            = h;

	TRACE_END();
	return res;
}

struct tex_patch
asset_tex_patch(i32 id, i32 x, i32 y, i32 w, i32 h, i32 ml, i32 mr, i32 mt, i32 mb)
{
	struct tex_patch res = {0};

	res.t   = asset_db_tex_get_by_id(&ASSETS.db, id).tex;
	res.r.x = x;
	res.r.y = y;
	res.r.w = w;
	res.r.h = h;
	res.ml  = ml;
	res.mr  = mr;
	res.mt  = mt;
	res.mb  = mb;

	return res;
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

str8
asset_path_to_full_path(struct str8 path)
{
	str8 res       = path;
	str8 base_path = sys_base_path();
	if(base_path.size == 0) { return res; }

	marena_reset(&ASSETS.scratch_marena);
	enum path_style path_style = path_style_from_str8(base_path);
	struct alloc scratch       = ASSETS.scratch_alloc;
	struct str8_list path_list = {0};
	str8_list_push(scratch, &path_list, base_path);
	str8_list_push(scratch, &path_list, path);
	res = path_join_by_style(scratch, &path_list, path_style);

	return res;
}
