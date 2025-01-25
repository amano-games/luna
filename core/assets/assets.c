#include "assets.h"
#include "arr.h"
#include "assets/fnt.h"
#include "gfx/gfx.h"
#include "mem-arena.h"
#include "sys-log.h"
#include "sys-assert.h"
#include "str.h"

void *asset_allocf(void *ctx, usize s);

void
assets_init(void *mem, usize size)
{
	log_info("Assets", "init");
	marena_init(&ASSETS.marena, mem, size);
	ASSETS.alloc       = (struct alloc){asset_allocf, (void *)&ASSETS};
	ASSETS.next_tex_id = NUM_TEX_ID;
	ASSETS.next_snd_id = NUM_SFX_ID;
}

void *
asset_allocf(void *ctx, usize s)
{
	struct assets *assets = (struct assets *)ctx;
	void *mem             = marena_alloc(&assets->marena, s);
	if(!mem) {
		usize left  = marena_size_rem(&ASSETS.marena);
		usize total = assets->marena.buf_size;
		usize used  = total - left;
		log_error("Assets", "Ran out of asset mem! requested: %u kb", (uint)s / 1024);
		log_error("Assets", "MEM: used: %u kb left: %u kb total: %u kb", (uint)used / 1024, (uint)left / 1024, (uint)total / 1024);
		BAD_PATH
	}

	return mem;
}

struct tex
asset_tex(i32 id)
{
	assert(0 <= id && id < NUM_TEX_ID_MAX);
	return ASSETS.tex[id].tex;
}

i32
asset_tex_get_id(str8 file_name)
{
	for(i32 i = 0; i < ASSETS.next_tex_id; i++) {
		struct asset_tex *at = &ASSETS.tex[i];
		if(str8_match(at->path, file_name, 0)) {
			return i;
		}
	}
	return 0;
}

i32
asset_tex_load(const str8 file_name, struct tex *tex)
{
	for(i32 i = 0; i < ASSETS.next_tex_id; i++) {
		struct asset_tex *at = &ASSETS.tex[i];
		if(str8_match(at->path, file_name, 0)) {
			if(tex) *tex = at->tex;
			return i;
		}
	}

	// TODO: use filepath arena for asset names
	str8 path_name = str8_fmt_push(ASSETS.alloc, "%s%s", FILE_PATH_TEX, file_name.str);

	log_info("Assets", "Load tex %s (%s)", file_name.str, path_name.str);

	struct tex t = tex_load(path_name, ASSETS.alloc);

	if(t.px == NULL) {
		log_info("Assets", "Lod failed %s (%s)", file_name.str, path_name.str);
		return -1;
	}

	i32 id              = ASSETS.next_tex_id++;
	ASSETS.tex[id].path = file_name;
	ASSETS.tex[id].tex  = t;

	if(tex) *tex = t;
	return id;
}

i32
asset_tex_load_id(i32 id, str8 file_name, struct tex *tex)
{
	assert(0 <= id && id < NUM_TEX_ID_MAX);
	// assert(ASSET_ALLOCATOR.ctx != NULL);
	str8 path_name = str8_fmt_push(ASSETS.alloc, "%s%s", FILE_PATH_TEX, file_name.str);

	log_info("Assets", "Load tex %s (%s)", file_name.str, path_name.str);

	struct tex t        = tex_load(path_name, ASSETS.alloc);
	ASSETS.tex[id].path = str8_cpy_push(ASSETS.alloc, file_name);
	ASSETS.tex[id].tex  = t;

	if(t.px) {
		if(tex) *tex = t;
		return id;
	}

	return -1;
}

i32
asset_tex_put(struct tex t)
{
	i32 id = ASSETS.next_tex_id++;
	asset_tex_put_id(id, t);
	return id;
}

struct tex
asset_tex_put_id(i32 id, struct tex t)
{
	assert(0 <= id && id < NUM_TEX_ID_MAX);
	ASSETS.tex[id].tex = t;
	return t;
}

struct tex_rec
asset_tex_rec(i32 id, i32 x, i32 y, i32 w, i32 h)
{
	struct tex_rec result = {0};

	result.t   = asset_tex(id);
	result.r.x = x;
	result.r.y = y;
	result.r.w = w;
	result.r.h = h;

	return result;
}

struct snd
asset_snd(i32 id)
{
	assert(0 <= id && id < NUM_SFX_ID_MAX);
	return ASSETS.snd[id].snd;
}

i32
asset_snd_load(const str8 path, struct snd *snd)
{
	for(i32 i = 0; i < ASSETS.next_snd_id; i++) {
		struct asset_snd *asset_snd = &ASSETS.snd[i];
		if(str8_match(asset_snd->path, path, 0)) {
			if(snd) *snd = asset_snd->snd;
			return i;
		}
	}

	log_info("Assets", "Load snd %s", path.str);

	struct snd s = snd_load(path, ASSETS.alloc);

	if(s.buf == NULL) {
		log_info("Assets", "Lod failed %s", path.str);
		return -1;
	}

	i32 id         = ASSETS.next_snd_id++;
	ASSETS.snd[id] = (struct asset_snd){
		.snd  = s,
		.path = path,
	};

	if(snd) *snd = s;
	return id;
}

i32
asset_snd_load_id(i32 id, str8 file_name, struct snd *snd)
{
	assert(0 <= id && id < NUM_SFX_ID_MAX);
	log_info("Assets", "Load snd %s", file_name.str);

	struct snd s       = snd_load(file_name, ASSETS.alloc);
	ASSETS.snd[id].snd = s;

	if(s.buf) {
		if(snd) *snd = s;
		return id;
	}

	return -1;
}

i32
asset_snd_get_id(str8 path)
{
	for(i32 i = 1; i < ASSETS.next_snd_id; i++) {
		struct asset_snd *at = &ASSETS.snd[i];
		if(str8_match(at->path, path, 0)) {
			return i;
		}
	}
	return 0;
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

struct fnt
assets_fnt_load(str8 path, struct alloc alloc, struct alloc scratch)
{
	struct fnt res = {0};
	str8 fnt_ext   = str8_lit(".fnt");

	res.widths                      = arr_ini(FNT_CHAR_MAX, sizeof(*res.widths), alloc);
	res.kern_pairs                  = arr_ini(FNT_KERN_PAIRS_MAX, sizeof(*res.kern_pairs), alloc);
	arr_header(res.widths)->len     = arr_cap(res.widths);
	arr_header(res.kern_pairs)->len = arr_cap(res.kern_pairs);
	mclr(res.widths, sizeof(*res.widths) * arr_len(res.widths));
	mclr(res.kern_pairs, sizeof(*res.kern_pairs) * arr_len(res.kern_pairs));

	assert(str8_ends_with(path, fnt_ext, 0));
	log_info("fnt", "Load fnt info: %s", path.str);
	struct sys_full_file_res file_res = sys_load_full_file(path, scratch);
	if(file_res.data == NULL) {
		log_error("fnt", "Failed loading fnt info: %s", path.str);
		return res;
	}
	char *data          = file_res.data;
	usize size          = file_res.size;
	struct ser_reader r = {
		.data = data,
		.len  = size,
	};

	fnt_read(&r, &res);

	str8 base_name = str8_chop_last_dot(path);
	str8 tex_path  = str8_fmt_push(scratch, "%.*s-table-%d-%d.tex", (i32)base_name.size, base_name.str, res.cell_w, res.cell_h);

	log_info("fnt", "Load tex: %s", tex_path.str);
	res.t = tex_load(tex_path, alloc);
	if(res.t.px == NULL) {
		log_error("fnt", "Failed loading tex info: %s", path.str);
		return res;
	}

	res.grid_w = res.t.w / res.cell_w;
	res.grid_h = res.t.h / res.cell_h;
	return res;
}
