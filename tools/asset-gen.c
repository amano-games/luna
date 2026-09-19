#include "base/base-inc.h"

#include <tinydir.h>
#include "base/cmd-line.h"
#include "base/log.h"
#include "base/marena.h"
#include "base/path.h"
#include "base/str.h"
#include "sys/sys-io.h"
#include "sys/sys.h"
#include "tools/aseprite/aseprite.h"
#include "tools/asset/asset.h"
#include "whereami.c"

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

#include "engine/animation/animation-db.c"
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
#define ANI_EXT           "lunass"
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
	dbg_check(sys_file_is_valid(out), "file-cpy-raw", "failed to open file to write %s", out_path.str);
	dbg_check(sys_file_w(out, in_res.data, in_res.size) == (ssize)in_res.size, "file-cpy-raw", "failed to write: %s", out_path.str);

	res = true;
	log_info("cpy", "%s -> %s", in_path.str, out_path.str);

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
		dbg_check(sys_file_w(out, buffer, (u32)n) == n, "asset-gen", "Failed to copy file", out_path.str);
	}

	res = true;
	log_info("cpy", "%s -> %s", in_path.str, out_path.str);

error:;
	if(sys_file_is_valid(in)) { sys_file_close(in); }
	if(sys_file_is_valid(out)) { sys_file_close(out); }
	return res;
}

void
asset_gen_recursive(
	const str8 in_dir,
	const str8 out_dir,
	struct marena *arena)
{

	struct alloc alloc = marena_allocator(arena);
	tinydir_dir *dir   = alloc_struct(alloc, dir);
	tinydir_open(dir, (char *)in_dir.str);

	while(dir->has_next) {
		tinydir_file file;
		tinydir_readfile(dir, &file);

		str8 file_name = str8_cstr(file.name);
		str8 in_path   = str8_fmt_push(alloc, "%.*s/%.*s", str8_spread(in_dir), str8_spread(file_name));
		str8 out_path  = str8_fmt_push(alloc, "%.*s/%.*s", str8_spread(out_dir), str8_spread(file_name));

		if(file.is_dir) {
			if(!str8_match(file_name, str8_lit("."), 0) && !str8_match(file_name, str8_lit(".."), 0)) {
				sys_make_dir(out_path);
				asset_gen_recursive(str8_cstr(file.path), out_path, arena);
			}
		} else {
			void *reset_p  = arena->p;
			str8 extension = str8_cstr(file.extension);
			if(str8_match(extension, str8_lit(IMG_EXT), 0)) {
				struct asset_blob blob = {0};
				png_to_tex_blob(in_path, alloc, sys_allocator(), &blob);
				str8 out_file_path = path_make_file_name_with_ext(alloc, out_path, str8_lit(TEX_EXT));
				b32 res            = asset_blob_w(blob, out_file_path);
				sys_free(blob.data);
			} else if(str8_match(extension, str8_lit(ASE_EXT), 0)) {
				b32 res = aseprite_to_assets(in_path, out_path, alloc);
			} else if(str8_match(extension, str8_lit(ANI_EXT), 0)) {
				b32 res = file_cpy(in_path, out_path);
			} else if(str8_match(extension, str8_lit(AUD_EXT), 0)) {
				b32 res = wav_to_snd(in_path, out_path, alloc);
			} else if(str8_match(extension, str8_lit(AI_EXT), 0)) {
				i32 res = handle_btree(in_path, out_path, alloc);
			} else if(str8_match(extension, str8_lit(FNT_EXT), 0)) {
				i32 res = handle_fnt_pd(in_path, out_path, alloc);
			} else if(str8_match(extension, str8_lit(ASSETS_DB_EXT), 0)) {
				i32 res = handle_tsj(in_path, out_path, alloc);
			} else if(str8_match(extension, str8_lit(PINBALL_TABLE_EXT), 0)) {
				i32 res = pinbtjson_handle(in_path, out_path);
			} else if(str8_match(extension, str8_lit(RAW_EXT), 0)) {
				b32 res = file_cpy_raw(in_path, out_path);
			}
			marena_reset_to(arena, reset_p);
		}

		tinydir_next(dir);
	}

	tinydir_close(dir);
}

int
main(int argc, char *argv[])
{
	int res                = EXIT_FAILURE;
	struct alloc alloc_sys = sys_allocator();

	usize mem_size = MMEGABYTE(1);
	void *mem      = mem_alloc_size(alloc_sys, mem_size);
	dbg_check_warn(mem, "asset-gen", "Failed to get scratch memory");
	struct marena scratch_arena = {0};
	marena_init(&scratch_arena, mem, mem_size);
	struct alloc scratch = marena_allocator(&scratch_arena);

	struct cmd_line cmd = cmd_line_from_argcv(scratch, argc, argv);
	b32 packed          = cmd_line_has_flag(&cmd, str8_lit("pack"));

	if(cmd.inputs.node_count < 2) {
		sys_printf("Usage: %.*s <in_path> <destination_path>", str8_spread(cmd.exe_name));
		res = EXIT_FAILURE;
		goto error;
	}

	str8 in_path  = cmd.inputs.first->str;
	str8 out_path = cmd.inputs.first->next->str;

	log_info("asset-gen", "Processing%s assets from %s -> %s", packed ? " packed" : "", in_path.str, out_path.str);
	dbg_check(sys_make_dir(out_path), "asset-gen", "failed to create folder %.*s", str8_spread(out_path));

	asset_gen_recursive(in_path, out_path, &scratch_arena);

	res = EXIT_SUCCESS;

error:;
	if(mem) {
		sys_free(mem);
	}

	return res;
}
