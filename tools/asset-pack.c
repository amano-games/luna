#include "base/base-inc.h"

#include <tinydir.h>
#include "base/cmd-line.h"
#include "base/dbg.h"
#include "base/hash.h"
#include "base/log.h"
#include "base/marena.h"
#include "base/mem.h"
#include "base/path.h"
#include "base/str.h"
#include "engine/assets/pck.h"
#include "sys/sys-io.h"
#include "sys/sys.h"
#include "whereami.c"

#include "sys/sys-inc.h"
#include "sys/sys-inc.c"

#include "lib/tex/tex.c"
#include "lz4/lz4.c"
#include "lz4/lz4hc.c"

#include "base/marena.c"
#include "base/str.c"
#include "base/cmd-line.c"
#include "base/path.c"

#define LOG_ID           "asset-pack"
#define DEFAULT_OUT_FILE "assets." PCK_EXT
#define PCK_LZ4HC_LEVEL  LZ4HC_CLEVEL_MAX

struct pck_w {
	ssize archive_size;
	ssize file_count;
	sys_file file;
	struct pck_file *files;
};

struct pck_stored {
	ssize size;
	ssize base_size;
	u16 flags;
};

static void
pck_u16w(u16 v, sys_file f)
{
	u8 b[sizeof(u16)];
	b[0] = 0xff & (v);
	b[1] = 0xff & (v >> 8);
	dbg_assert(sys_file_w(f, b, sizeof(u16)) == (ssize)sizeof(u16));
}

static void
pck_u32w(u32 v, sys_file f)
{
	u8 b[sizeof(u32)];
	b[0] = 0xff & (v);
	b[1] = 0xff & (v >> 8);
	b[2] = 0xff & (v >> 16);
	b[3] = 0xff & (v >> 24);
	dbg_assert(sys_file_w(f, b, sizeof(u32)) == (ssize)sizeof(u32));
}

static void
pck_u64w(u64 v, sys_file f)
{
	u8 b[sizeof(u64)];
	b[0] = 0xff & (v);
	b[1] = 0xff & (v >> 8);
	b[2] = 0xff & (v >> 16);
	b[3] = 0xff & (v >> 24);
	b[4] = 0xff & (v >> 32);
	b[5] = 0xff & (v >> 40);
	b[6] = 0xff & (v >> 48);
	b[7] = 0xff & (v >> 56);
	dbg_assert(sys_file_w(f, b, sizeof(u64)) == (ssize)sizeof(u64));
}

static int
pck_lz4(const u8 *src, ssize size, u8 *out, ssize out_cap)
{
	int n     = 0;
	int bound = 0;

	if(size <= 0 || size > LZ4_MAX_INPUT_SIZE) {
		goto done;
	}

	bound = LZ4_compressBound((int)size);
	if(bound <= 0 || out_cap < (ssize)bound) {
		goto done;
	}

	n = LZ4_compress_HC(
		(const char *)src,
		(char *)out,
		(int)size,
		(int)out_cap,
		PCK_LZ4HC_LEVEL);

	// Bare block must shrink vs the uncompressed member.
	if(n <= 0 || n >= (int)size) {
		n = 0;
	}

done:
	return n;
}

static struct pck_stored
pck_store_file(struct alloc alloc, str8 disk_path, str8 pack_path, sys_file dst)
{
	struct pck_stored res = {.size = -1, .flags = PCK_FLAG_NONE};
	sys_file src          = sys_file_zero();
	u8 *data              = NULL;
	u8 *lz4               = NULL;
	usize f_size          = 0;
	int bound             = 0;
	int n                 = 0;
	b32 should_compress   = false;

	src = sys_file_open_r(disk_path);
	dbg_check(sys_file_is_valid(src), LOG_ID, "failed to open file: %.*s", str8_spread(disk_path));

	sys_file_seek_end(src, 0);
	f_size = (usize)sys_file_tell(src);
	sys_file_seek_set(src, 0);

	// Empty files are valid
	if(f_size == 0) {
		res.size      = 0;
		res.base_size = 0;
		goto error;
	}

	data = mem_alloc_size(alloc, f_size);
	dbg_check(data, LOG_ID, "failed alloc for %.*s", str8_spread(disk_path));
	dbg_check(sys_file_r(src, data, (u32)f_size) == (ssize)f_size, LOG_ID, "failed to read file: %.*s", str8_spread(disk_path));

#if defined(ASSET_PACK_COMPRESSION)
	if(str8_ends_with(pack_path, str8_lit("." TEX_EXT), 0) && f_size <= (usize)LZ4_MAX_INPUT_SIZE) {
		should_compress = true;
	}
#endif

	if(should_compress) {
		bound = LZ4_compressBound((int)f_size);
		if(bound > 0) {
			lz4 = mem_alloc_size(alloc, (usize)bound);
			dbg_check(lz4, LOG_ID, "lz4 alloc failed");
			n = pck_lz4(data, (ssize)f_size, lz4, bound);
		}
	}

	if(n > 0) {
		dbg_check(sys_file_w(dst, lz4, (u32)n) == (ssize)n, LOG_ID, "failed writing lz4 %.*s", str8_spread(disk_path));
		res.size      = (ssize)n;
		res.base_size = (ssize)f_size;
		res.flags     = PCK_FLAG_COMPRESSED_LZ4;
	} else {
		dbg_check(sys_file_w(dst, data, (u32)f_size) == (ssize)f_size, LOG_ID, "failed to copy file %.*s", str8_spread(disk_path));
		res.size      = (ssize)f_size;
		res.base_size = (ssize)f_size;
	}

error:;
	if(sys_file_is_valid(src)) {
		sys_file_close(src);
	}
	return res;
}

static b32
pck_collect_paths(
	struct alloc alloc,
	struct str8_list *list,
	str8 root,
	str8 rel,
	struct alloc scratch)
{
	b32 res         = false;
	tinydir_dir dir = {0};
	str8 dir_path;

	if(rel.size == 0) {
		dir_path = root;
	} else {
		dir_path = str8_fmt_push(scratch, "%.*s/%.*s", str8_spread(root), str8_spread(rel));
	}

	dbg_check(tinydir_open(&dir, (char *)dir_path.str) == 0, LOG_ID, "failed to open dir %.*s", str8_spread(dir_path));

	while(dir.has_next) {
		tinydir_file file;
		tinydir_readfile(&dir, &file);

		str8 name = str8_cstr(file.name);
		str8 child_rel;

		if(rel.size == 0) {
			child_rel = str8_fmt_push(alloc, "%.*s", str8_spread(name));
		} else {
			child_rel = str8_fmt_push(alloc, "%.*s/%.*s", str8_spread(rel), str8_spread(name));
		}

		if(file.is_dir) {
			if(
				!str8_match(name, str8_lit("."), 0) &&
				!str8_match(name, str8_lit(".."), 0)) {
				dbg_check(
					pck_collect_paths(alloc, list, root, child_rel, scratch),
					LOG_ID,
					"failed collecting %.*s",
					str8_spread(child_rel));
			}
		} else {
			str8_list_push(alloc, list, child_rel);
		}

		tinydir_next(&dir);
	}

	res = true;

error:;
	tinydir_close(&dir);
	return res;
}

static b32
pck_pack_file(struct pck_w *pack, str8 root, str8 path, struct alloc scratch)
{
	b32 res = false;
	u8 zero = 0;
	u16 path_len;
	u64 hash;
	struct pck_stored stored;
	str8 disk_path = str8_fmt_push(scratch, "%.*s/%.*s", str8_spread(root), str8_spread(path));

	hash     = hash_fnv1a_str8(path);
	path_len = (u16)(path.size + 1);

	dbg_check(sys_file_w(pack->file, path.str, (u32)path.size) == (ssize)path.size, LOG_ID, "failed writing path %.*s", str8_spread(path));
	dbg_check(sys_file_w(pack->file, &zero, 1) == 1, LOG_ID, "failed writing path null");

	stored = pck_store_file(scratch, disk_path, path, pack->file);
	dbg_check(stored.size >= 0, LOG_ID, "failed copying %.*s", str8_spread(disk_path));

	log_info(LOG_ID, "%6d %016llx %_$$10u %.*s", pack->file_count, hash, (u32)stored.size, str8_spread(path));
	if(stored.flags & PCK_FLAG_COMPRESSED_LZ4) {
		log_info(LOG_ID, "        lz4 %_$$u -> %_$$u", (u32)stored.base_size, (u32)stored.size);
	}

	pack->files[pack->file_count] = (struct pck_file){
		.hash      = hash,
		.offset    = pack->archive_size,
		.size      = stored.size,
		.base_size = stored.base_size,
		.path_len  = path_len,
		.flags     = stored.flags,
	};

	pack->archive_size += stored.size + path_len;
	pack->file_count++;

	res = true;

error:;
	return res;
}

static b32
pck_pack(struct alloc alloc, str8 input_path, str8 out_path)
{
	b32 res                      = false;
	struct pck_w pack            = {0};
	struct str8_list paths       = {0};
	struct alloc scratch         = {0};
	struct marena scratch_marena = {0};
	ssize total_size;
	ssize i;

	{
		usize mem_size = MMEGABYTE(4);
		void *mem      = mem_alloc_size(sys_allocator(), mem_size);
		dbg_check_warn(mem, LOG_ID, "Failed to get scratch memory");
		marena_init(&scratch_marena, mem, mem_size);
		scratch = marena_allocator(&scratch_marena);
	}

	pack.file = sys_file_open_w(out_path);
	dbg_check(sys_file_is_valid(pack.file), LOG_ID, "failed to open file: %.*s", str8_spread(out_path));

	dbg_check(
		pck_collect_paths(alloc, &paths, input_path, str8_lit(""), scratch),
		LOG_ID,
		"failed collecting paths");
	pack.files = alloc_arr(alloc, pack.files, paths.node_count);

	for(struct str8_node *node = paths.first; node != NULL; node = node->next) {
		void *reset_p = scratch_marena.p;
		dbg_check(pck_pack_file(&pack, input_path, node->str, scratch), LOG_ID, "failed packing %.*s", str8_spread(node->str));
		marena_reset_to(&scratch_marena, reset_p);
	}

	total_size = pack.archive_size + PCK_HEADER_SIZE;
	for(i = 0; i < pack.file_count; ++i) {
		pck_u64w(pack.files[i].hash, pack.file);
		pck_u32w((u32)pack.files[i].offset, pack.file);
		pck_u32w((u32)pack.files[i].size, pack.file);
		pck_u32w((u32)pack.files[i].base_size, pack.file);
		pck_u16w(pack.files[i].path_len, pack.file);
		pck_u16w(pack.files[i].flags, pack.file);
		total_size += PCK_INDEX_SIZE;
	}

	pck_u32w((u32)pack.file_count, pack.file);
	pck_u32w((u32)total_size, pack.file);
	pck_u32w(PCK_MAGIC, pack.file);

	res = true;
	log_info(LOG_ID, "files: %d, size: %_$$u", pack.file_count, (u32)total_size);

error:;
	if(sys_file_is_valid(pack.file)) {
		sys_file_close(pack.file);
	}
	if(scratch_marena.buf) {
		sys_free(scratch_marena.buf);
	}
	return res;
}

int
main(int argc, char *argv[])
{
	int res                = EXIT_FAILURE;
	struct alloc alloc_sys = sys_allocator();
	struct alloc alloc     = {0};
	struct marena marena   = {0};
	str8 in_path;
	str8 out_path;

	{
		usize mem_size = MMEGABYTE(4);
		void *mem      = mem_alloc_size(alloc_sys, mem_size);
		dbg_check_warn(mem, LOG_ID, "Failed to get arena memory");
		marena_init(&marena, mem, mem_size);
	}

	alloc               = marena_allocator(&marena);
	struct cmd_line cmd = cmd_line_from_argcv(alloc, argc, argv);

	if(cmd.inputs.node_count < 1) {
		sys_printf(
			"Usage: %.*s <input-folder|input-pck> [output-folder|output-pck]\n"
			"  output defaults to %s\n"
			"  example: %.*s ./tmp/assets/files ./tmp/assets.pck\n",
			str8_spread(cmd.exe_name),
			DEFAULT_OUT_FILE,
			str8_spread(cmd.exe_name));
		res = EXIT_FAILURE;
		goto error;
	}

	in_path  = cmd.inputs.first->str;
	out_path = str8_lit(DEFAULT_OUT_FILE);

	if(cmd.inputs.node_count >= 2) {
		out_path = cmd.inputs.first->next->str;
	}

	log_info(LOG_ID, "Packing assets from %s -> %s", in_path.str, out_path.str);
	dbg_check(pck_pack(alloc, in_path, out_path), LOG_ID, "pack failed");

	res = EXIT_SUCCESS;

error:;
	if(marena.buf) {
		sys_free(marena.buf);
	}

	return res;
}
