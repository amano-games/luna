#include "base/base-inc.h"

#include <tinydir.h>
#include "base/cmd-line.h"
#include "base/dbg.h"
#include "base/ht.h"
#include "sys/sys-log.h"
#include "base/marena.h"
#include "base/path.h"
#include "base/str.h"
#include "lib/color.h"
#include "sys/sys-io.h"
#include "sys/sys.h"
#include "tools/aseprite/aseprite.h"
#include "tools/asset/asset.h"

#include "sys/sys-inc.h"
#include "sys/sys-inc.c"
#include "sys/sys-lz4hc.c"

#include "base/marena.c"
#include "base/str.c"
#include "base/cmd-line.c"
#include "base/path.c"

#include "lib/bet/bet-ser.c"
#include "lib/bet/bet.c"
#include "lib/rndm.c"
#include "lib/tex/tex.c"
#include "lib/pinb/pinb-ser.c"
#include "lib/fnt/fnt.c"

#include "engine/animation/animation-clips.c"
#include "engine/audio/adpcm.c"
#include "engine/physics/physics.c"
#include "engine/physics/body-ser.c"

#include "./wav/wav.h"
#include "./wav/wav.c"
#include "./png/png.c"
#include "./aseprite/aseprite.c"

#include "tools/asset/asset.c"
#include "tools/btree/btree.h"
#include "tools/btree/btree.c"
#include "tools/tsj/tsj.h"
#include "tools/tsj/tsj.c"
#include "engine/assets/tex-atlas.c"
#include "engine/assets/path-db.c"
#include "tools/fnt-pd/fnt-pd.c"
#include "tools/fnt-pd/fnt-pd.h"
#include "tools/pinbtjson/pinbtjson.h"
#include "tools/pinbtjson/pinbtjson.c"

#include "engine/collisions/collisions.c"
#include "engine/collisions/collisions-ser.c"

#define RAW_EXT           "raw"
#define IMG_EXT           "png"
#define ASE_EXT           "aseprite"
#define AUD_EXT           "wav"
#define AI_EXT            "btree"
#define FNT_EXT           "fnt"
#define ASSETS_DB_EXT     "tsj"
#define PINBALL_TABLE_EXT "pinbjson"

b32
file_cpy_raw(const str8 in_path, const str8 out_path)
{
	b32 res                         = false;
	struct sys_full_file_res in_res = sys_load_full_file(sys_allocator(), in_path);
	sys_file out                    = sys_file_open_w(out_path);
	dbg_check(sys_file_is_valid(out), "file-cpy-raw", "failed to open file to write %.*s", str8_spread(out_path));
	dbg_check(sys_file_w(out, in_res.data, in_res.size) == (ssize)in_res.size, "file-cpy-raw", "failed to write: %.*s", str8_spread(out_path));

	res = true;
	log_info("cpy", "%.*s -> %.*s", str8_spread(in_path), str8_spread(out_path));

error:;
	if(in_res.data) { sys_free(in_res.data); }
	if(sys_file_is_valid(out)) { sys_file_close(out); }
	return res;
}

b32
file_cpy(const str8 in_path, const str8 out_path)
{
	b32 res      = false;
	sys_file in  = sys_file_open_r(in_path);
	sys_file out = sys_file_open_w(out_path);
	char buffer[7192];
	ssize n;

	while((n = sys_file_r(in, buffer, sizeof(buffer))) > 0) {
		dbg_check(sys_file_w(out, buffer, (u32)n) == n, "asset-gen", "Failed to copy file %.*s", str8_spread(out_path));
	}

	res = true;
	log_info("cpy", "%.*s -> %.*s", str8_spread(in_path), str8_spread(out_path));

error:;
	if(sys_file_is_valid(in)) { sys_file_close(in); }
	if(sys_file_is_valid(out)) { sys_file_close(out); }
	return res;
}

void
asset_gen_recursive(
	const str8 in_dir,
	const str8 out_dir,
	const str8 src_root,
	const str8 dest_root,
	struct marena *arena,
	struct gfx_col_lut *lut,
	enum asset_flag flag)
{
	void *dir_reset_p  = arena->p;
	struct alloc alloc = marena_allocator(arena);
	tinydir_dir *dir   = alloc_struct(alloc, dir);
	tinydir_open(dir, (char *)in_dir.str);

	while(dir->has_next) {
		void *entry_reset_p = arena->p;
		tinydir_file file;
		tinydir_readfile(dir, &file);

		str8 file_name = str8_cstr(file.name);
		str8 in_path   = str8_fmt_push(alloc, "%.*s/%.*s", str8_spread(in_dir), str8_spread(file_name));
		str8 out_path  = str8_fmt_push(alloc, "%.*s/%.*s", str8_spread(out_dir), str8_spread(file_name));

		if(file.is_dir) {
			if(!str8_match(file_name, str8_lit("."), 0) && !str8_match(file_name, str8_lit(".."), 0)) {
				sys_make_dir(out_path);
				asset_gen_recursive(str8_cstr(file.path), out_path, src_root, dest_root, arena, lut, flag);
			}
		} else {
			void *reset_p  = arena->p;
			str8 extension = str8_cstr(file.extension);
			if(str8_match(extension, str8_lit(IMG_EXT), 0)) {
				struct asset_blob blob = {0};
				png_to_tex_blob(in_path, alloc, sys_allocator(), &blob, lut, flag);
				str8 out_file_path = path_make_file_name_with_ext(alloc, out_path, str8_lit(TEX_EXT));
				b32 res            = asset_blob_w(blob, out_file_path);
				sys_free(blob.data);
			} else if(str8_match(extension, str8_lit(ASE_EXT), 0)) {
				b32 res = aseprite_to_assets(in_path, out_path, alloc, flag);
			} else if(str8_match(extension, str8_lit(AUD_EXT), 0)) {
				b32 res = wav_to_snd(in_path, out_path, alloc);
			} else if(str8_match(extension, str8_lit(AI_EXT), 0)) {
				i32 res = handle_btree(in_path, out_path, alloc);
			} else if(str8_match(extension, str8_lit(FNT_EXT), 0)) {
				i32 res = handle_fnt_pd(in_path, out_path, alloc);
			} else if(str8_match(extension, str8_lit(ASSETS_DB_EXT), 0)) {
				i32 res = handle_tsj(in_path, out_path, src_root, dest_root, alloc);
			} else if(str8_match(extension, str8_lit(PINBALL_TABLE_EXT), 0)) {
				i32 res = pinbtjson_handle(in_path, out_path);
			} else if(str8_match(extension, str8_lit(RAW_EXT), 0)) {
				b32 res = file_cpy_raw(in_path, out_path);
			}
			marena_reset_to(arena, reset_p);
		}

		marena_reset_to(arena, entry_reset_p);
		tinydir_next(dir);
	}

	tinydir_close(dir);
	marena_reset_to(arena, dir_reset_p);
}

struct gfx_col_lut
asset_gen_load_palette_map(str8 path)
{
	struct gfx_col_lut res = {0};
	dbg_check(path.size > 0, "asset-gen", "palette path empty");

	i32 w, h, n;
	stbi_uc *data = stbi_load((char *)path.str, &w, &h, &n, 4);
	dbg_check(data, "asset-gen", "palette failed to load: %.*s", str8_spread(path));
	dbg_check(h < 4 && w > 2 && w < (U8_MAX - 1), "asset-gen", "palette wrong dimensions (%dx%d): %.*s", w, h, str8_spread(path));

	struct gfx_col_palette *out = &res.out;
	out->len                    = 1;
	for(ssize x = 0; x < w; ++x) {
		const stbi_uc *p = data + x * 4;
		u32 color        = ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | (u32)p[3];

		if(p[3] == 0) { break; } // palette_out stops after first transparent pixel

		// Entry 0 in palette is always transparent
		out->colors[out->len++] = color;
	}

	struct gfx_col_palette *in  = &res.in;
	struct gfx_col_palette *map = &res.map;
	map->len                    = w + 1;
	in->len                     = w + 1;
	i32 stride                  = w * 4;
	for(ssize y = 1; y < h; ++y) {
		for(ssize x = 0; x < w; ++x) {
			const stbi_uc *p = data + (stride * y) + x * 4;
			u32 color        = ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | (u32)p[3];
			switch(y) {
			case 1: {
				// Entry 0 in palette is always transparent
				in->colors[x + 1] = color;
			} break;
			case 2: {
				// search for the color in the already stored palette_out,
				// if it matches map it
				b32 found = false;
				for(ssize i = 1; i < out->len; ++i) {
					if(out->colors[i] == color) {
						map->colors[x + 1] = i;
						found              = true;
						break;
					}
				}
				dbg_check(found, "asset-gen", "color in map row not found");
			} break;
			}
		}
	}

error:;
	if(data) {
		stbi_image_free(data);
	}
	return res;
}

int
main(int argc, char *argv[])
{
	int res                = EXIT_FAILURE;
	struct alloc alloc_sys = sys_allocator();

	usize mem_size = MMEGABYTE(2);
	void *mem      = mem_alloc_size(alloc_sys, mem_size);
	dbg_check_warn(mem, "asset-gen", "Failed to get scratch memory");
	struct marena scratch_arena = {0};
	marena_init(&scratch_arena, mem, mem_size);
	struct alloc scratch = marena_allocator(&scratch_arena);

	struct cmd_line cmd  = cmd_line_from_argcv(scratch, argc, argv);
	b32 packed           = cmd_line_has_flag(&cmd, str8_lit("pack"));
	enum asset_flag flag = cmd_line_has_flag(&cmd, str8_lit("compress"))
		? ASSET_FLAG_LZ4HC
		: ASSET_FLAG_NONE;

	if(cmd.inputs.node_count < 2) {
		sys_printf("Usage: %.*s [--compress] <in_path> <destination_path>", str8_spread(cmd.exe_name));
		res = EXIT_FAILURE;
		goto error;
	}

	struct gfx_col_lut lut = {0};
	str8 palette_path      = cmd_line_str8(&cmd, str8_lit("palette"));
	if(palette_path.size > 0) {
		lut    = asset_gen_load_palette_map(palette_path);
		lut.ht = ht_new_u32(ht_exp_from_count(250), scratch);
	}
	log_info("asset-gen", "color palette: %ld", lut.out.len);

	str8 in_path  = cmd.inputs.first->str;
	str8 out_path = cmd.inputs.first->next->str;

	log_info("asset-gen", "Processing%s assets from %.*s -> %.*s", packed ? " packed" : "", str8_spread(in_path), str8_spread(out_path));
	dbg_check(sys_make_dirs(out_path), "asset-gen", "failed to create folder %.*s", str8_spread(out_path));

	asset_gen_recursive(in_path, out_path, in_path, out_path, &scratch_arena, &lut, flag);

	res = EXIT_SUCCESS;

error:;
	if(mem) {
		sys_free(mem);
	}

	return res;
}
