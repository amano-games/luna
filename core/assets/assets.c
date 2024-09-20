#include "assets.h"
#include "gfx.h"
#include "mem-arena.h"
#include "sys-log.h"
#include "sys-assert.h"
#include "str.h"

void *asset_allocf(void *ctx, usize s);

void
assets_init(void *mem, usize size)
{
	log_info("Assets", "init");
	{
		marena_init(&ASSETS.marena, mem, size);
		ASSETS.alloc       = (struct alloc){asset_allocf, (void *)&ASSETS};
		ASSETS.next_tex_id = NUM_TEX_ID;
	}
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
		if(str8_match(at->file_name, file_name, 0)) {
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
		if(str8_match(at->file_name, file_name, 0)) {
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

	i32 id                   = ASSETS.next_tex_id++;
	ASSETS.tex[id].file_name = file_name;
	ASSETS.tex[id].tex       = t;

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

	struct tex t             = tex_load(path_name, ASSETS.alloc);
	ASSETS.tex[id].file_name = str8_cpy_push(ASSETS.alloc, file_name);
	ASSETS.tex[id].tex       = t;

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
