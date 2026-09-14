#pragma once

#include "base/types.h"
#include "engine/assets/asset-db.h"

#include "engine/assets/qop.h"
#include "engine/gfx/gfx.h"
#include "base/mem.h"
#include "base/marena.h"
#include "tools/asset/asset-defs.h"

struct assets {
	struct tex display;

	struct asset_db db;

	struct qop_desc qop;
	void *qop_ht;
	str8 pack_path;

	struct marena marena;
	struct alloc alloc;
};

struct asset_stream {
	struct qop_desc qop;
	struct qop_file *file;
	ssize cursor;
	b32 open;
};

extern struct assets ASSETS;
struct alloc assets_allocator(struct assets *assets);

void assets_ini(struct alloc alloc, usize size);
void assets_qop_ini(struct alloc scratch, str8 path);
void assets_qop_close(void);

struct asset_blob asset_blob_from_handle(struct alloc scratch, struct asset_handle handle);
struct asset_blob asset_blob_read(struct alloc scratch, str8 path);
i32 asset_file_read_ex(str8 path, u8 *dest, ssize start, ssize len);

b32 asset_stream_open(struct asset_stream *s, struct asset_handle handle);
void asset_stream_close(struct asset_stream *s);
b32 asset_stream_is_open(struct asset_stream *s);
i32 asset_stream_read(struct asset_stream *s, void *dest, ssize len);
void asset_stream_seek(struct asset_stream *s, ssize off);

struct tex asset_tex(i32 id);
struct tex asset_tex_from_handle(struct alloc alloc, struct asset_handle handle);
struct tex asset_tex_read(struct alloc alloc, str8 path);
i32 asset_tex_load(str8 path, struct tex *tex);
i32 asset_tex_get_id(str8 path);

struct fnt asset_fnt(i32 id);
i32 asset_fnt_load(struct alloc scratch, str8 path, struct fnt *fnt);
i32 asset_fnt_get_id(str8 path);

struct snd asset_snd(i32 id);
struct snd asset_snd_from_handle(struct alloc alloc, struct asset_handle handle);
struct snd asset_snd_read(struct alloc alloc, str8 path);
i32 asset_snd_load(str8 path, struct snd *snd);
i32 asset_snd_get_id(str8 path);

struct bet asset_bet(i32 id);
i32 asset_bet_load(struct alloc scratch, str8 path, struct bet *bet);
i32 asset_bet_get_id(str8 path);

struct tex_rec asset_tex_rec(i32 id, i32 x, i32 y, i32 w, i32 h);
struct tex_patch asset_tex_patch(i32 id, i32 x, i32 y, i32 w, i32 h, i32 ml, i32 mr, i32 mt, i32 mb);
enum asset_type asset_path_get_type(str8 path);
str8 asset_path_to_full_path(struct alloc scratch, struct str8 path);
