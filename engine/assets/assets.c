#include "assets.h"
#include "base/dbg.h"
#include "base/hash.h"
#include "base/mem.h"
#include "engine/assets/asset-db.h"
#include "lib/bet/bet-ser.h"
#include "lib/fnt/fnt.h"
#include "engine/gfx/gfx.h"
#include "base/marena.h"
#include "base/path.h"
#include "base/log.h"
#include "base/str.h"
#include "base/types.h"

struct assets ASSETS;

void *asset_allocf(void *ctx, ssize size, ssize align);

void
assets_ini(struct alloc alloc, usize size)
{
	log_info("assets", "init");
	void *mem = mem_alloc_size(alloc, size);
	marena_init(&ASSETS.marena, mem, size);
	ASSETS.alloc   = (struct alloc){asset_allocf, (void *)&ASSETS};
	ASSETS.display = tex_frame_buffer();
	mclr_struct(&ASSETS.db);
}

void
assets_qop_ini(struct alloc scratch, str8 path)
{
	str8 pack = asset_path_to_full_path(scratch, path);
	dbg_check(qop_open(pack, &ASSETS.qop) != 0, "assets", "qop open failed");

	// Keep a permanent copy so music can open a second pack handle later.
	ASSETS.pack_path = str8_cpy_push(ASSETS.alloc, pack);
	dbg_check(ASSETS.pack_path.str, "assets", "qop path copy failed");

	ASSETS.qop_ht = mem_alloc_size(ASSETS.alloc, ASSETS.qop.hashmap_size);
	dbg_check(ASSETS.qop_ht, "assets", "qop ht alloc failed");
	dbg_check(qop_read_index(&ASSETS.qop, ASSETS.qop_ht) != 0, "assets", "qop index failed");
error:;
}

void
assets_qop_close(void)
{
	if(sys_file_is_valid(ASSETS.qop.fh)) {
		qop_close(&ASSETS.qop);
	}
	ASSETS.qop_ht    = NULL;
	ASSETS.pack_path = str8_zero();
}

struct asset_blob
asset_blob_from_handle(struct alloc scratch, struct asset_handle handle)
{
	struct asset_blob res = {0};
	struct qop_file *f    = qop_find_hash(&ASSETS.qop, handle.path_hash);

	dbg_check(f, "assets", "failed to find file hash %016llx", handle.path_hash);

	void *data = mem_alloc_size(scratch, (usize)f->size);
	dbg_check(data, "assets", "failed to alloc file hash %016llx", handle.path_hash);

	i32 size = qop_read(&ASSETS.qop, f, data);
	dbg_check(size == f->size, "assets", "qop size doesn't match: %d %d", size, (i32)f->size);

	res.size = f->size;
	res.data = data;

error:;
	return res;
}

struct asset_blob
asset_blob_read(struct alloc scratch, str8 path)
{
	return asset_blob_from_handle(scratch, asset_db_handle_from_path(path, ASSET_TYPE_NONE));
}

i32
asset_file_read_ex(str8 path, u8 *dest, ssize start, ssize len)
{
	i32 res                    = 0;
	struct asset_handle handle = asset_db_handle_from_path(path, ASSET_TYPE_NONE);
	struct qop_file *f         = qop_find_hash(&ASSETS.qop, handle.path_hash);

	dbg_check(f, "assets", "failed to find file: %.*s", str8_spread(path));
	res = qop_read_ex(&ASSETS.qop, f, dest, start, len);
error:;
	return res;
}

b32
asset_stream_open(struct asset_stream *s, struct asset_handle handle)
{
	b32 res            = false;
	struct qop_file *f = qop_find_hash(&ASSETS.qop, handle.path_hash);

	dbg_check(f, "assets", "stream find failed hash %016llx", handle.path_hash);
	dbg_check(ASSETS.pack_path.size, "assets", "pack path missing");

	// Own seek cursor via a second open; share the read-only index.
	dbg_check(qop_open(ASSETS.pack_path, &s->qop) != 0, "assets", "stream qop open failed");
	s->qop.ht          = ASSETS.qop.ht;
	s->qop.hashmap_len = ASSETS.qop.hashmap_len;
	s->file            = f;
	s->cursor          = 0;
	s->open            = true;
	res                = true;

error:;
	if(!res && s) {
		if(sys_file_is_valid(s->qop.fh)) {
			qop_close(&s->qop);
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
	if(sys_file_is_valid(s->qop.fh)) {
		qop_close(&s->qop);
	}
	mclr_struct(s);
}

b32
asset_stream_is_open(struct asset_stream *s)
{
	return s && s->open && sys_file_is_valid(s->qop.fh);
}

i32
asset_stream_read(struct asset_stream *s, void *dest, ssize len)
{
	i32 n = 0;
	dbg_check(asset_stream_is_open(s), "assets", "stream not open");
	n = qop_read_ex(&s->qop, s->file, dest, s->cursor, len);
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

struct tex
asset_tex_from_handle(struct alloc alloc, struct asset_handle handle)
{
	struct tex res           = {0};
	struct tex_header header = {0};
	struct qop_file *f       = qop_find_hash(&ASSETS.qop, handle.path_hash);

	dbg_check(f, "assets", "failed to find tex hash %016llx", handle.path_hash);
	dbg_check(f->size >= (ssize)sizeof(header), "assets", "tex too small hash %016llx", handle.path_hash);

	i32 n = qop_read_ex(&ASSETS.qop, f, (u8 *)&header, 0, (ssize)sizeof(header));
	dbg_check(n == (i32)sizeof(header), "assets", "tex header read failed hash %016llx", handle.path_hash);
	dbg_check(header.fmt == TEX_FMT_OPAQUE || header.fmt == TEX_FMT_MASK,
		"assets",
		"invalid tex fmt %u hash %016llx",
		header.fmt,
		handle.path_hash);
	dbg_check(header.w > 0 && header.h > 0,
		"assets",
		"invalid tex size %ux%u hash %016llx",
		header.w,
		header.h,
		handle.path_hash);

	if(header.fmt == TEX_FMT_MASK) {
		res = tex_create(alloc, (i32)header.w, (i32)header.h);
	} else {
		res = tex_create_opaque(alloc, (i32)header.w, (i32)header.h);
	}
	dbg_check(res.px, "assets", "tex alloc failed hash %016llx", handle.path_hash);

	ssize tex_size = (ssize)sizeof(u32) * res.wword * res.h;
	dbg_check(f->size >= (ssize)sizeof(header) + tex_size,
		"assets",
		"tex truncated hash %016llx",
		handle.path_hash);

	n = qop_read_ex(&ASSETS.qop, f, (u8 *)res.px, (ssize)sizeof(header), tex_size);
	dbg_check(n == (i32)tex_size, "assets", "tex pixels read failed hash %016llx", handle.path_hash);

error:;
	return res;
}

struct tex
asset_tex_read(struct alloc alloc, str8 path)
{
	return asset_tex_from_handle(alloc, asset_db_handle_from_path(path, ASSET_TYPE_TEXTURE));
}

i32
asset_tex_load(str8 path, struct tex *tex)
{
	i32 res = asset_tex_get_id(path);
	if(res != 0 && tex) {
		*tex = asset_tex(res);
	}
	dbg_check_warn(res == 0, "assets", "Tex already loaded: %.*s", str8_spread(path));

	res          = -1;
	struct tex t = asset_tex_read(ASSETS.alloc, path);

	dbg_check(t.px, "assets", "failed to load tex: %.*s", str8_spread(path));

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
	f.t            = asset_tex_read(ASSETS.alloc, tex_path);
	dbg_check(f.t.px, "assets", "failed to load fnt tex: %.*s", str8_spread(tex_path));

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
	struct qop_file *f           = qop_find_hash(&ASSETS.qop, handle.path_hash);

	dbg_check(f, "assets", "failed to find snd hash %016llx", handle.path_hash);
	dbg_check(f->size >= (ssize)sizeof(u32), "assets", "snd too small hash %016llx", handle.path_hash);

	i32 n = qop_read_ex(&ASSETS.qop, f, (u8 *)&snd_header, 0, (ssize)sizeof(u32));
	dbg_check(n == (i32)sizeof(u32), "assets", "snd header read failed hash %016llx", handle.path_hash);

	u32 bytes = (snd_header.sample_count + 1) >> 1;
	dbg_check(f->size >= (ssize)sizeof(u32) + (ssize)bytes,
		"assets",
		"snd truncated hash %016llx",
		handle.path_hash);

	u8 *buf = alloc_size_aligned(alloc, bytes, alignof(u8), false);
	dbg_check(buf, "assets", "snd alloc failed hash %016llx", handle.path_hash);

	n = qop_read_ex(&ASSETS.qop, f, buf, (ssize)sizeof(u32), (ssize)bytes);
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
