#include "assets.h"
#include "arr.h"
#include "gfx.h"
#include "json.h"
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

struct snd
asset_snd(i32 id)
{
	assert(0 <= id && id < NUM_SFX_ID_MAX);
	return ASSETS.snd[id].snd;
}

i32
asset_snd_load(const str8 file_name, struct snd *snd)
{
	// for(i32 i = 0; i < ASSETS.next_snd_id; i++) {
	// 	struct asset_snd *asset_snd = &ASSETS.snd[i];
	// 	if(str8_match(asset_snd->file_name, file_name, 0)) {
	// 		if(tex) *tex = at->tex;
	// 		return i;
	// 	}
	// }

	log_info("Assets", "Load snd %s", file_name.str);

	struct snd s = snd_load(file_name, ASSETS.alloc);

	if(snd->buf == NULL) {
		log_info("Assets", "Lod failed %s", file_name.str);
		return -1;
	}

	i32 id             = ASSETS.next_snd_id++;
	ASSETS.snd[id].snd = s;

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

struct fnt
fnt_load(const str8 path, struct alloc alloc, struct alloc scratch)
{

	struct fnt fnt = {0};
	char *txt;
	fnt.widths = (u8 *)alloc.allocf(alloc.ctx, sizeof(u8) * 256);
	if(!fnt.widths) {
		log_error("Assets", "allocating font memory");
		return fnt;
	}
	str8 tex_ext  = str8_lit(".tex");
	str8 json_ext = str8_lit(".fnt");

	// replace .fnt with .tex
	str8 filename_tex = str8_cpy_push(scratch, path);
	str8 filename_ext = {.str = &filename_tex.str[filename_tex.size - json_ext.size], .size = json_ext.size};

	assert(str8_ends_with(path, json_ext, 0));
	str8 json = {0};
	log_info("Assets", "Load fnt info: %s", path.str);

	bool32 loaded_json = json_load(path, scratch, &json);
	str8_cpy(&tex_ext, &filename_ext);

	log_info("Assets", "Load fnt tex: %s", filename_tex.str);

	fnt.t = tex_load(filename_tex, alloc);

	{
		jsmn_parser parser;
		jsmn_init(&parser);
		i32 token_count = jsmn_parse(&parser, (char *)json.str, json.size, NULL, 0);
		jsmn_init(&parser);
		jsmntok_t *tokens = arr_ini(token_count, sizeof(jsmntok_t), scratch);
		i32 json_res      = jsmn_parse(&parser, (char *)json.str, json.size, tokens, token_count);
		assert(json_res == token_count);

		for(i32 i = 1; i < token_count; i++) {
			jsmntok_t *key   = &tokens[i];
			jsmntok_t *value = &tokens[i + 1];
			if(json_eq(json, key, str8_lit("grid_width")) == 0) {
				assert(value->type == JSMN_PRIMITIVE);
				fnt.grid_w = json_parse_i32(json, value);
			}
			if(json_eq(json, key, str8_lit("grid_height")) == 0) {
				assert(value->type == JSMN_PRIMITIVE);
				fnt.grid_h = json_parse_i32(json, value);
			}
			if(json_eq(json, key, str8_lit("glyph_widths")) == 0) {
				assert(value->type == JSMN_ARRAY);
				for(i32 j = 0; j < value->size; j++) {
					jsmntok_t *item = &tokens[i + j + 2];
					fnt.widths[j]   = json_parse_i32(json, item);
				}
			}
		}
	}

	return fnt;
}

enum asset_type
asset_path_get_type(str8 path)
{
	str8 tex_ext = str8_lit(".tex");
	str8 snd_ext = str8_lit(".snd");
	str8 fnt_ext = str8_lit(".fnt");

	if(str8_ends_with(path, tex_ext, 0)) {
		return ASSET_TYPE_TEXTURE;
	} else if(str8_ends_with(path, snd_ext, 0)) {
		return ASSET_TYPE_SOUND;
	} else if(str8_ends_with(path, fnt_ext, 0)) {
		return ASSET_TYPE_FONT;
	}

	return 0;
}
