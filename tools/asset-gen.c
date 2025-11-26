#include <tinydir.h>
#include "sys/sys-io.h"
#include "tools/aseprite/aseprite.h"
#include "whereami.c"

#include "sys/sys.h"
#include "sys/sys-cli.c"
#include "sys/sys-io.c"

#include "base/dbg.h"
#include "base/marena.h"
#include "base/mem.h"
#include "base/types.h"
#include "base/mem.c"
#include "base/marena.c"
#include "base/str.h"
#include "base/str.c"
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
	struct sys_full_file_res in_res = sys_load_full_file(in_path, sys_allocator());
	void *out                       = sys_file_open_w(out_path);
	dbg_check(out, "file-cpy-raw", "failed to open file to write %s", out_path.str);
	dbg_check(sys_file_w(out, in_res.data, in_res.size), "file-cpy-raw", "failed to write: %s", out_path.str);

	res = true;
	log_info("cpy", "%s -> %s", in_path.str, out_path.str);

error:;
	if(in_res.data) { sys_free(in_res.data); }
	if(out) { sys_file_close(out); }
	return res;
}

b32
file_cpy(const str8 in_path, const str8 out_path)
{
	b32 res   = false;
	void *in  = sys_file_open_r(in_path);
	void *out = sys_file_open_w(out_path);
	char buffer[BUFSIZ];
	ssize n;

	while((n = sys_file_r(in, buffer, sizeof(buffer)) > 0)) {
		dbg_check(sys_file_w(out, buffer, n), "asset-gen", "Failed to copy file", out_path.str);
	}

	res = true;
	log_info("cpy", "%s -> %s", in_path.str, out_path.str);

error:;
	if(in) { sys_file_close(in); }
	if(out) { sys_file_close(out); }
	return res;
}

void
handle_asset_recursive(
	const str8 in_dir,
	const str8 out_dir,
	struct marena *arena)
{

	struct alloc alloc = marena_allocator(arena);
	tinydir_dir dir;
	tinydir_open(&dir, (char *)in_dir.str);

	while(dir.has_next) {
		tinydir_file file;
		tinydir_readfile(&dir, &file);

		str8 file_name = str8_cstr(file.name);
		str8 in_path   = str8_fmt_push(alloc, "%s/%s", in_dir.str, file.name);
		str8 out_path  = str8_fmt_push(alloc, "%s/%s", out_dir.str, file.name);

		if(file.is_dir) {
			if(!str8_match(file_name, str8_lit("."), 0) && !str8_match(file_name, str8_lit(".."), 0)) {
				sys_make_dir(out_path);
				handle_asset_recursive(str8_cstr(file.path), out_path, arena);
			}
		} else {
			void *reset_p  = arena->p;
			str8 extension = str8_cstr(file.extension);
			if(str8_match(extension, str8_lit(IMG_EXT), 0)) {
				b32 res = png_to_tex(in_path, out_path, alloc);
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

		tinydir_next(&dir);
	}

	tinydir_close(&dir);
}

int
main(int argc, char *argv[])
{
	int res = EXIT_FAILURE;

	if(argc != 3) {
		log_info("asset-gen", "Usage: %s <in_path> <destination_path>", argv[0]);
		res = EXIT_SUCCESS;
		return res;
	}

	str8 in_path  = str8_cstr(argv[1]);
	str8 out_path = str8_cstr(argv[2]);
	log_info("asset-gen", "Processing assets from %s -> %s", in_path.str, out_path.str);

	str8 path = str8_cstr(argv[2]);
	// TODO: check if folder exists
	sys_make_dir(path);

	usize mem_size = MMEGABYTE(1);
	u8 *mem        = sys_alloc(NULL, mem_size);
	dbg_check_warn(mem, "asset-gen", "Failed to get scratch memory");
	struct marena arena = {0};
	marena_init(&arena, mem, mem_size);

	handle_asset_recursive(in_path, out_path, &arena);

	res = EXIT_SUCCESS;

error:;
	if(mem) {
		sys_free(mem);
	}

	return res;
}
