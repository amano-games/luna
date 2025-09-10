#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include "whereami.c"

#include "sys.h"
#include "sys-cli.c"

#include "base/marena.h"
#include "base/mem.h"
#include "base/types.h"
#include "base/mem.c"
#include "base/marena.c"
#include "lib/bet/bet.c"
#include "engine/animation/animation-db.c"
#include "lib/bet/bet-ser.c"
#include "lib/rndm.c"
#include "base/str.h"
#include "base/str.c"
#include "base/path.c"
#include "tex/tex.c"

#include "engine/audio/adpcm.c"
#include "./wav/wav.h"
#include "./wav/wav.c"
#include "./png/png.c"

#include "engine/physics/physics.c"
#include "engine/physics/body-ser.c"
#include "tools/btree/btree.h"
#include "tools/btree/btree.c"
#include "tools/tsj/tsj.h"
#include "tools/tsj/tsj.c"
#include "tools/fnt-pd/fnt-pd.c"
#include "tools/fnt-pd/fnt-pd.h"
#include "tools/pinbtjson/pinbtjson.h"
#include "tools/pinbtjson/pinbtjson.c"
#include "pinb/pinb-ser.c"
#include "lib/fnt/fnt.c"
#include "engine/collisions/collisions.c"
#include "engine/collisions/collisions-ser.c"
#include "sys-io.c"

#define IMG_EXT ".png"
#define AUD_EXT ".wav"
// #define RAW_EXT ".raw"
#define ANI_EXT           ".lunass"
#define AI_EXT            ".btree"
#define FNT_EXT           ".fnt"
#define ASSETS_DB_EXT     ".tsj"
#define PINBALL_TABLE_EXT ".pinbjson"

void
fcopy(const str8 in_path, const str8 out_path)
{
	FILE *f1 = fopen((char *)in_path.str, "rb");
	FILE *f2 = fopen((char *)out_path.str, "wb");
	char buffer[BUFSIZ];
	size_t n;

	while((n = fread(buffer, sizeof(char), sizeof(buffer), f1)) > 0) {
		if(fwrite(buffer, n, sizeof(char), f2) != 1) {
			log_error("asset-gen", "Failed to copy file \n");
		}
	}

	log_info("asset-gen", "[cpy] %s -> %s\n", in_path.str, out_path.str);
}

void
handle_asset_recursive(
	const str8 in_dir,
	const str8 out_dir,
	struct alloc scratch)
{

	struct marena *marena = (struct marena *)scratch.ctx;

	tinydir_dir dir;
	tinydir_open(&dir, (char *)in_dir.str);

	while(dir.has_next) {
		tinydir_file file;
		tinydir_readfile(&dir, &file);

		// char in_path_buff[FILENAME_MAX];
		// char out_path_buff[FILENAME_MAX];
		// str8 in_path  = str8_array_fixed(in_path_buff);
		// str8 out_path = str8_array_fixed(out_path_buff);
		str8 in_path  = str8_fmt_push(scratch, "%s/%s", in_dir.str, file.name);
		str8 out_path = str8_fmt_push(scratch, "%s/%s", out_dir.str, file.name);
		// stbsp_snprintf((char *)in_path.str, in_path.size, "%s/%s", in_dir.str, file.name);
		// stbsp_snprintf((char *)out_path.str, out_path.size, "%s/%s", out_dir.str, file.name);

		if(file.is_dir) {
			if(strcmp(file.name, ".") != 0 && strcmp(file.name, "..") != 0) {
				mkdir((char *)out_path.str, file._s.st_mode);
				handle_asset_recursive(str8_cstr(file.path), out_path, scratch);
			}
		} else {
			void *p = marena->p;
			if(strstr(file.name, IMG_EXT) != NULL) {
				handle_texture(in_path, out_path, scratch);
			} else if(strstr(file.name, ANI_EXT) != NULL) {
				fcopy(in_path, out_path);
			} else if(strstr(file.name, AUD_EXT)) {
				i32 res = handle_wav(in_path, out_path, scratch);
			} else if(strstr(file.name, AI_EXT)) {
				i32 res = handle_btree(in_path, out_path, scratch);
			} else if(strstr(file.name, FNT_EXT)) {
				i32 res = handle_fnt_pd(in_path, out_path, scratch);
			} else if(strstr(file.name, ASSETS_DB_EXT)) {
				i32 res = handle_tsj(in_path, out_path, scratch);
			} else if(strstr(file.name, PINBALL_TABLE_EXT)) {
				i32 res = pinbtjson_handle(in_path, out_path);
			} else {
				// fcopy(in_path, out_path);
			}
			marena_reset_to(marena, p);
		}

		tinydir_next(&dir);
	}

	tinydir_close(&dir);
}

int
main(int argc, char *argv[])
{
	if(argc != 3) {
		log_info("asset-gen", "Usage: %s <in_path> <destination_path>\n", argv[0]);
		return 1;
	}

	log_info("asset-gen", "Processing assets...\n");
	mkdir(argv[2], 0755);

	usize scratch_mem_size = MMEGABYTE(1);
	u8 *scratch_mem_buffer = sys_alloc(NULL, scratch_mem_size);
	dbg_assert(scratch_mem_buffer != NULL);
	struct marena scratch_marena = {0};
	marena_init(&scratch_marena, scratch_mem_buffer, scratch_mem_size);
	struct alloc scratch = marena_allocator(&scratch_marena);

	handle_asset_recursive(str8_cstr(argv[1]), str8_cstr(argv[2]), scratch);

	sys_free(scratch_mem_buffer);

	return EXIT_SUCCESS;
}
