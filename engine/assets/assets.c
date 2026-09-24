#include "assets.h"
#include "base/dbg.h"
#include "base/hash.h"
#include "base/mem.h"
#include "base/arr.h"
#include "engine/assets/asset-db.h"
#include "engine/assets/tex-atlas.h"
#include "engine/animation/animation-clips.h"
#include "lib/bet/bet-ser.h"
#include "lib/fnt/fnt.h"
#include "engine/gfx/gfx.h"
#include "base/marena.h"
#include "base/path.h"
#include "base/log.h"
#include "base/str.h"
#include "base/types.h"
#include "lib/tex/tex.h"
#include "sys/sys-lz4.h"
#include "sys/sys.h"

struct assets ASSETS;

void *asset_allocf(void *ctx, ssize size, ssize align);

void
assets_ini(struct alloc alloc, usize size)
{
	log_info("assets", "init");
	void *mem = mem_alloc_size(alloc, size);
	marena_init(&ASSETS.marena, mem, size);
	ASSETS.alloc = (struct alloc){asset_allocf, (void *)&ASSETS};
	mclr_struct(&ASSETS.db);
}

void
assets_pck_ini(struct alloc scratch, str8 path)
{
	str8 pack = asset_path_to_full_path(scratch, path);
	dbg_check(pck_open(pack, &ASSETS.pck) != 0, "assets", "pck open failed");

	// Keep a permanent copy so music can open a second pack handle later.
	ASSETS.pack_path = str8_cpy_push(ASSETS.alloc, pack);
	dbg_check(ASSETS.pack_path.str, "assets", "pck path copy failed");

	ASSETS.pck_ht = mem_alloc_size(ASSETS.alloc, ASSETS.pck.ht_size);
	dbg_check(ASSETS.pck_ht, "assets", "pck ht alloc failed");
	dbg_check(pck_read_index(&ASSETS.pck, ASSETS.pck_ht) != 0, "assets", "pck index failed");
error:;
}

void
assets_pck_close(void)
{
	if(sys_file_is_valid(ASSETS.pck.fh)) {
		pck_close(&ASSETS.pck);
	}
	ASSETS.pck_ht    = NULL;
	ASSETS.pack_path = str8_zero();
}

struct asset_blob
asset_blob_from_handle(struct alloc alloc, struct asset_handle handle)
{
	struct asset_blob res = {0};
	struct pck_file *f    = pck_find_hash(&ASSETS.pck, handle.path_hash);
	void *data            = NULL;
	i32 size              = 0;

	dbg_check(f, "assets", "failed to find file hash %016llx", handle.path_hash);
	dbg_assert(!(f->flags & PCK_FLAG_COMPRESSED_LZ4));

	data = mem_alloc_size(alloc, (usize)f->size);
	dbg_check(data, "assets", "failed to alloc file hash %016llx", handle.path_hash);

	size = pck_read(&ASSETS.pck, f, data);
	dbg_check(size == f->size, "assets", "pck size doesn't match: %d %d", size, (i32)f->size);

	res.size = f->size;
	res.data = data;

error:;
	return res;
}

struct asset_blob
asset_blob_read(struct alloc alloc, str8 path)
{
	return asset_blob_from_handle(alloc, asset_db_handle_from_path(path, ASSET_TYPE_NONE));
}

i32
asset_file_read_ex(str8 path, u8 *dest, ssize start, ssize len)
{
	i32 res                    = 0;
	struct asset_handle handle = asset_db_handle_from_path(path, ASSET_TYPE_NONE);
	struct pck_file *f         = pck_find_hash(&ASSETS.pck, handle.path_hash);

	dbg_check(f, "assets", "failed to find file: %.*s", str8_spread(path));
	res = pck_read_ex(&ASSETS.pck, f, dest, start, len);
error:;
	return res;
}

b32
asset_stream_open(struct asset_stream *s, struct asset_handle handle)
{
	b32 res            = false;
	struct pck_file *f = pck_find_hash(&ASSETS.pck, handle.path_hash);

	dbg_check(f, "assets", "stream find failed hash %016llx", handle.path_hash);
	dbg_assert(!(f->flags & PCK_FLAG_COMPRESSED_LZ4));
	dbg_check(ASSETS.pack_path.size, "assets", "pack path missing");
	dbg_check(pck_open(ASSETS.pack_path, &s->pck) != 0, "assets", "stream pck open failed");

	s->pck.ht     = ASSETS.pck.ht;
	s->pck.ht_len = ASSETS.pck.ht_len;
	s->file       = f;
	s->cursor     = 0;
	dbg_check(pck_seek(&s->pck, f, 0) == 0, "assets", "stream seek failed");
	s->open = true;
	res     = true;

error:;
	if(!res && s) {
		if(sys_file_is_valid(s->pck.fh)) {
			pck_close(&s->pck);
		}
		mclr_struct(s);
	}
	return res;
}

void
asset_stream_close(struct asset_stream *s)
{
	if(!s || !s->open) {
		return;
	}
	if(sys_file_is_valid(s->pck.fh)) {
		pck_close(&s->pck);
	}
	mclr_struct(s);
}

b32
asset_stream_is_open(struct asset_stream *s)
{
	return s && s->open && sys_file_is_valid(s->pck.fh);
}

i32
asset_stream_read(struct asset_stream *s, void *dest, ssize len)
{
	i32 n = 0;
	dbg_check(asset_stream_is_open(s), "assets", "stream not open");
	n = pck_read_cur(&s->pck, (u8 *)dest, len);
	if(n > 0) {
		s->cursor += n;
	}
error:;
	return n;
}

void
asset_stream_seek(struct asset_stream *s, ssize off)
{
	dbg_check(asset_stream_is_open(s), "assets", "stream not open");
	dbg_check(pck_seek(&s->pck, s->file, off) == 0, "assets", "stream seek failed");
	s->cursor = off;
error:;
}

void *
asset_allocf(void *ctx, ssize size, ssize align)
{
	struct assets *assets = (struct assets *)ctx;
	void *mem             = marena_alloc(&assets->marena, size, align);
	dbg_check_mem(mem != NULL, "assets");
	return mem;

error:;
	log_error("assets", "Ran out of asset mem! requested: %$u", (uint)size);
	MARENA_LOG_USAGE(&ASSETS.marena, "assets");
	return NULL;
}

struct tex
asset_tex(i32 id)
{
	struct asset_tex res = asset_db_tex_get_by_id(&ASSETS.db, id);
	return res.tex;
}

i32
asset_tex_get_id(str8 path)
{
	i32 res = asset_db_tex_get_id(&ASSETS.db, (struct asset_handle){
												  .path_hash = hash_fnv1a_str8(path),
												  .type      = ASSET_TYPE_TEXTURE,
											  });
	return res;
}

static inline struct tex
asset_tex_from_pck(struct alloc alloc, struct alloc scratch, struct pck_file *f)
{
	struct tex res           = {0};
	struct tex_header header = {0};
	u8 *px_src               = NULL;
	i32 n                    = 0;
	ssize header_size        = (ssize)sizeof(struct tex_header);
	ssize tex_size           = 0;
	ssize packed_len         = 0;

	dbg_assert(!(f->flags & PCK_FLAG_COMPRESSED_LZ4));
	dbg_assert(f->size == f->base_size);
	dbg_check(f->size >= header_size, "assets", "tex too small hash %016llx", f->hash);

	n = pck_read_ex(&ASSETS.pck, f, (u8 *)&header, 0, header_size);
	dbg_check(n == (i32)header_size, "assets", "tex header read failed hash %016llx", f->hash);
	dbg_check(header.fmt == TEX_FMT_1B_OPAQUE || header.fmt == TEX_FMT_1B_MASK || header.fmt == TEX_FMT_8B_INDEX,
		"assets",
		"invalid tex fmt %u hash %016llx",
		header.fmt,
		f->hash);
	dbg_check(header.w > 0 && header.h > 0,
		"assets",
		"invalid tex size %ux%u hash %016llx",
		header.w,
		header.h,
		f->hash);
	dbg_check((header.flags & ~TEX_FLAG_LZ4) == 0,
		"assets",
		"invalid tex flags %u hash %016llx",
		header.flags,
		f->hash);

	res = tex_create(alloc, (i32)header.w, (i32)header.h, header.fmt);
	dbg_check(res.px1b, "assets", "tex alloc failed hash %016llx", f->hash);

	tex_size   = (ssize)sizeof(u32) * res.wword * res.h;
	packed_len = f->size - header_size;
	dbg_check(packed_len > 0, "assets", "tex pixels missing hash %016llx", f->hash);

	if(header.flags & TEX_FLAG_LZ4) {
		px_src = mem_alloc_size(scratch, (usize)packed_len);
		dbg_check(px_src, "assets", "scratch too small for packed tex hash %016llx", f->hash);
		n = pck_read_ex(&ASSETS.pck, f, px_src, header_size, packed_len);
		dbg_check(n == (i32)packed_len, "assets", "tex packed read failed hash %016llx", f->hash);
		n = sys_lz4_decompress(px_src, res.px1b, packed_len, tex_size);
		dbg_check(n == (int)tex_size, "assets", "tex lz4 decode failed hash %016llx", f->hash);
	} else {
		dbg_check(packed_len >= tex_size, "assets", "tex truncated hash %016llx", f->hash);
		n = pck_read_ex(&ASSETS.pck, f, (u8 *)res.px1b, header_size, tex_size);
		dbg_check(n == (i32)tex_size, "assets", "tex pixels read failed hash %016llx", f->hash);
	}

error:;
	return res;
}

struct tex
asset_tex_from_handle(struct alloc alloc, struct alloc scratch, struct asset_handle handle)
{
	struct tex res     = {0};
	struct pck_file *f = pck_find_hash(&ASSETS.pck, handle.path_hash);

	dbg_check(f, "assets", "failed to find tex hash %016llx", handle.path_hash);
	res = asset_tex_from_pck(alloc, scratch, f);

error:;
	return res;
}

struct tex
asset_tex_read(struct alloc alloc, struct alloc scratch, str8 path)
{
	return asset_tex_from_handle(alloc, scratch, asset_db_handle_from_path(path, ASSET_TYPE_TEXTURE));
}

i32
asset_atlas_load(struct alloc scratch, str8 tex_path, struct tex tex)
{
	i32 res                = 0;
	str8 atlas_path        = path_make_file_name_with_ext(scratch, tex_path, str8_lit(ATLAS_EXT));
	struct pck_file *f     = pck_find(&ASSETS.pck, atlas_path);
	struct tex_atlas atlas = {0};
	void *data             = NULL;

	if(f == NULL) {
		atlas = (struct tex_atlas){
			.cell_w = (u16)tex.w,
			.cell_h = (u16)tex.h,
		};
	} else {
		data = mem_alloc_size(scratch, (usize)f->size);
		dbg_check(data, "assets", "atlas alloc failed %.*s", str8_spread(atlas_path));
		dbg_check(
			pck_read(&ASSETS.pck, f, data) == f->size,
			"assets",
			"atlas read failed %.*s",
			str8_spread(atlas_path));
		atlas = atlas_from_mem(data, f->size);
	}

	res = (i32)asset_db_tex_atlas_push(&ASSETS.db, tex_path, atlas);

error:;
	return res;
}

i32
asset_ani_load(struct alloc scratch, str8 tex_path)
{
	i32 res                           = 0;
	str8 ani_path                     = path_make_file_name_with_ext(scratch, tex_path, str8_lit(ANI_EXT));
	struct pck_file *f                = pck_find(&ASSETS.pck, ani_path);
	void *data                        = NULL;
	struct ser_reader r               = {0};
	struct animation_clip *clips      = NULL;
	struct animation_clip *first_clip = NULL;
	struct animation_slice slice      = {0};
	ssize i;

	if(f != NULL) {
		data = mem_alloc_size(scratch, (usize)f->size);
		dbg_check(data, "assets", "ani alloc failed %.*s", str8_spread(ani_path));
		dbg_check(
			pck_read(&ASSETS.pck, f, data) == f->size,
			"assets",
			"ani read failed %.*s",
			str8_spread(ani_path));
		r     = (struct ser_reader){.data = data, .len = (int)f->size};
		clips = ani_clips_read(&r, scratch);

		first_clip = ASSETS.db.animations.data + arr_len(ASSETS.db.animations.data);
		for(i = 0; i < arr_len(clips); ++i) {
			dbg_assert(clips[i].count != 0);
			asset_db_animation_clip_push(&ASSETS.db, clips[i]);
		}

		slice = (struct animation_slice){
			.clip = first_clip,
			.size = arr_len(clips),
		};
		res = (i32)asset_db_animation_slice_push(&ASSETS.db, tex_path, slice);
		log_info("assets", "ani slice for: %s size: %d", tex_path.str, (int)slice.size);
	}

error:;
	return res;
}

i32
asset_tex_load(struct alloc scratch, str8 path, struct tex *tex)
{
	i32 res = asset_tex_get_id(path);
	if(res != 0 && tex) {
		*tex = asset_tex(res);
	}
	dbg_check_warn(res == 0, "assets", "Tex already loaded: %.*s", str8_spread(path));

	res          = -1;
	struct tex t = asset_tex_read(ASSETS.alloc, scratch, path);

	dbg_check(t.px1b, "assets", "failed to load tex: %.*s", str8_spread(path));

	log_info("assets", "Tex loaded: %s", path.str);
	res = asset_db_tex_push(&ASSETS.db, path, t);
	if(tex) {
		*tex = t;
	}

error:;
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
												  .path_hash = hash_fnv1a_str8(path),
												  .type      = ASSET_TYPE_FONT,
											  });
	return res;
}

i32
asset_fnt_load(struct alloc scratch, str8 path, struct fnt *fnt)
{
	i32 res = asset_fnt_get_id(path);
	if(res != 0 && fnt) {
		*fnt = asset_fnt(res);
	}
	dbg_check_warn(res == 0, "assets", "Fnt already loaded: %.*s", str8_spread(path));

	res                    = -1;
	struct asset_blob blob = asset_blob_read(scratch, path);
	struct fnt f           = fnt_load_from_mem(ASSETS.alloc, blob.data, blob.size);

	dbg_check(f.widths, "assets", "failed to load fnt: %.*s", str8_spread(path));

	// Companion atlas lives next to the .fnt member in the pack.
	str8 base_name = str8_chop_last_dot(path);
	str8 tex_path  = str8_fmt_push(scratch, "%.*s-table-%d-%d.tex", str8_spread(base_name), f.cell_w, f.cell_h);
	f.t            = asset_tex_read(ASSETS.alloc, scratch, tex_path);
	dbg_check(f.t.px1b, "assets", "failed to load fnt tex: %.*s", str8_spread(tex_path));

	f.grid_w = f.t.w / f.cell_w;
	f.grid_h = f.t.h / f.cell_h;

	log_info("assets", "Load fnt %s", path.str);
	res = asset_db_fnt_push(&ASSETS.db, path, f);
	if(fnt) *fnt = f;

error:;
	return res;
}

struct snd
asset_snd(i32 id)
{
	struct asset_snd res = asset_db_snd_get_by_id(&ASSETS.db, id);
	return res.snd;
}

struct snd
asset_snd_from_handle(struct alloc alloc, struct asset_handle handle)
{
	struct snd res               = {0};
	struct snd_header snd_header = {0};
	struct pck_file *f           = pck_find_hash(&ASSETS.pck, handle.path_hash);

	dbg_check(f, "assets", "failed to find snd hash %016llx", handle.path_hash);
	dbg_check(f->size >= (ssize)sizeof(u32), "assets", "snd too small hash %016llx", handle.path_hash);

	i32 n = pck_read_ex(&ASSETS.pck, f, (u8 *)&snd_header, 0, (ssize)sizeof(u32));
	dbg_check(n == (i32)sizeof(u32), "assets", "snd header read failed hash %016llx", handle.path_hash);

	u32 bytes = (snd_header.sample_count + 1) >> 1;
	dbg_check(f->size >= (ssize)sizeof(u32) + (ssize)bytes,
		"assets",
		"snd truncated hash %016llx",
		handle.path_hash);

	u8 *buf = alloc_size_aligned(alloc, bytes, alignof(u8), false);
	dbg_check(buf, "assets", "snd alloc failed hash %016llx", handle.path_hash);

	n = pck_read_ex(&ASSETS.pck, f, buf, (ssize)sizeof(u32), (ssize)bytes);
	dbg_check(n == (i32)bytes, "assets", "snd samples read failed hash %016llx", handle.path_hash);

	res.buf = buf;
	res.len = snd_header.sample_count;

error:;
	return res;
}

struct snd
asset_snd_read(struct alloc alloc, str8 path)
{
	return asset_snd_from_handle(alloc, asset_db_handle_from_path(path, ASSET_TYPE_SOUND));
}

i32
asset_snd_load(str8 path, struct snd *snd)
{
	i32 res = asset_snd_get_id(path);
	if(res != 0 && snd) {
		*snd = asset_snd(res);
	}
	dbg_check_warn(res == 0, "assets", "Snd already loaded: %.*s", str8_spread(path));

	res          = -1;
	struct snd s = asset_snd_read(ASSETS.alloc, path);

	dbg_check(s.len, "assets", "failed to load snd: %.*s", str8_spread(path));
	log_info("assets", "Load snd %s", path.str);
	res = asset_db_snd_push(&ASSETS.db, path, s);
	if(snd) *snd = s;

error:;
	return res;
}

i32
asset_snd_get_id(str8 path)
{
	i32 res = asset_db_snd_get_id(&ASSETS.db, (struct asset_handle){
												  .path_hash = hash_fnv1a_str8(path),
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
asset_bet_load(struct alloc scratch, str8 path, struct bet *bet)
{
	i32 res = asset_bet_get_id(path);
	if(res != 0 && bet) {
		*bet = asset_bet(res);
	}
	dbg_check_warn(res == 0, "assets", "Bet already loaded: %.*s", str8_spread(path));

	res                    = -1;
	struct asset_blob blob = asset_blob_read(scratch, path);
	struct bet b           = bet_load_from_mem(ASSETS.alloc, blob.data, (usize)blob.size);

	dbg_check(b.nodes, "assets", "Bet loading failed: %.*s", str8_spread(path));
	log_info("assets", "Bet loaded: %s", path.str);
	res = asset_db_bet_push(&ASSETS.db, path, b);
	if(bet) *bet = b;

error:;
	return res;
}

i32
asset_bet_get_id(str8 path)
{
	i32 res = asset_db_bet_get_id(&ASSETS.db, (struct asset_handle){
												  .path_hash = hash_fnv1a_str8(path),
												  .type      = ASSET_TYPE_BET,
											  });
	return res;
}

struct tex_rec
asset_tex_rec(i32 id, i32 x, i32 y, i32 w, i32 h)
{
	struct tex_rec res = {0};
	res.t              = asset_db_tex_get_by_id(&ASSETS.db, id).tex;
	res.r.x            = x;
	res.r.y            = y;
	res.r.w            = w;
	res.r.h            = h;
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
asset_path_to_full_path(struct alloc scratch, struct str8 path)
{
	str8 res       = path;
	str8 base_path = sys_base_path();
	if(base_path.size == 0) { return res; }

	enum path_style path_style = path_style_from_str8(base_path);
	struct str8_list path_list = {0};
	str8_list_push(scratch, &path_list, base_path);
	str8_list_push(scratch, &path_list, path);
	res = path_join_by_style(scratch, &path_list, path_style);

	return res;
}
