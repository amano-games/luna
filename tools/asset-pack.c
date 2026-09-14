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
#include "engine/assets/qop.h"
#include "sys/sys-io.h"
#include "sys/sys.h"
#include "whereami.c"

#include "sys/sys-inc.h"
#include "sys/sys-inc.c"

#include "lib/tex/tex.c"

#include "base/marena.c"
#include "base/str.c"
#include "base/cmd-line.c"
#include "base/path.c"

#define LOG_ID           "asset-pack"
#define DEFAULT_OUT_FILE "assets.qop"
#define QOP_INDEX_SIZE   20

struct qop_w {
	ssize archive_size;
	ssize file_count;
	sys_file file;
	struct qop_file *files;
};

static void
qop_u16w(u16 v, sys_file f)
{
	u8 b[sizeof(u16)];
	b[0] = 0xff & (v);
	b[1] = 0xff & (v >> 8);
	dbg_assert(sys_file_w(f, b, sizeof(u16)) == (ssize)sizeof(u16));
}

static void
qop_u32w(u32 v, sys_file f)
{
	u8 b[sizeof(u32)];
	b[0] = 0xff & (v);
	b[1] = 0xff & (v >> 8);
	b[2] = 0xff & (v >> 16);
	b[3] = 0xff & (v >> 24);
	dbg_assert(sys_file_w(f, b, sizeof(u32)) == (ssize)sizeof(u32));
}

static void
qop_u64w(u64 v, sys_file f)
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

// Copy file bytes into archive; return byte count or -1 on failure.
static ssize
qop_copy_into(str8 path, sys_file dst)
{
	ssize res   = -1;
	sys_file src = sys_file_zero();
	void *data  = NULL;
	usize f_size;

	src = sys_file_open_r(path);
	dbg_check(sys_file_is_valid(src), LOG_ID, "failed to open file: %.*s", str8_spread(path));

	sys_file_seek_end(src, 0);
	f_size = (usize)sys_file_tell(src);
	sys_file_seek_set(src, 0);

	// Empty files are valid archive members.
	if(f_size == 0) {
		res = 0;
		goto error;
	}

	data = mem_alloc_size(sys_allocator(), f_size);
	dbg_check(data, LOG_ID, "failed alloc for %.*s", str8_spread(path));
	dbg_check(sys_file_r(src, data, (u32)f_size) == (ssize)f_size, LOG_ID, "failed to read file: %.*s", str8_spread(path));
	dbg_check(sys_file_w(dst, data, (u32)f_size) == (ssize)f_size, LOG_ID, "failed to copy file %.*s", str8_spread(path));

	res = (ssize)f_size;

error:;
	if(data) {
		sys_free(data);
	}
	if(sys_file_is_valid(src)) {
		sys_file_close(src);
	}
	return res;
}

static b32
qop_collect_paths(
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
					qop_collect_paths(alloc, list, root, child_rel, scratch),
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
qop_pack_file(struct qop_w *qop, str8 root, str8 path, struct alloc scratch)
{
	b32 res = false;
	u8 zero = 0;
	u16 path_len;
	u64 hash;
	ssize size;
	str8 disk_path = str8_fmt_push(scratch, "%.*s/%.*s", str8_spread(root), str8_spread(path));

	hash     = hash_fnv1a_str8(path);
	path_len = (u16)(path.size + 1);

	dbg_check(sys_file_w(qop->file, path.str, (u32)path.size) == (ssize)path.size, LOG_ID, "failed writing path %.*s", str8_spread(path));
	dbg_check(sys_file_w(qop->file, &zero, 1) == 1, LOG_ID, "failed writing path null");

	size = qop_copy_into(disk_path, qop->file);
	dbg_check(size >= 0, LOG_ID, "failed copying %.*s", str8_spread(disk_path));

	log_info(LOG_ID, "%6d %016llx %_$$10u %.*s", qop->file_count, hash, (u32)size, str8_spread(path));

	qop->files[qop->file_count] = (struct qop_file){
		.hash     = hash,
		.offset   = qop->archive_size,
		.size     = size,
		.path_len = path_len,
		.flags    = QOP_FLAG_NONE,
	};

	qop->archive_size += size + path_len;
	qop->file_count++;

	res = true;

error:;
	return res;
}

static b32
qop_pack(struct alloc alloc, str8 input_path, str8 out_path, struct alloc scratch)
{
	b32 res                = false;
	struct qop_w qop       = {0};
	struct str8_list paths = {0};
	ssize total_size;
	ssize i;

	qop.file = sys_file_open_w(out_path);
	dbg_check(sys_file_is_valid(qop.file), LOG_ID, "failed to open file: %.*s", str8_spread(out_path));

	dbg_check(
		qop_collect_paths(alloc, &paths, input_path, str8_lit(""), scratch),
		LOG_ID,
		"failed collecting paths");
	qop.files = alloc_arr(alloc, qop.files, paths.node_count);

	for(struct str8_node *node = paths.first; node != NULL; node = node->next) {
		dbg_check(qop_pack_file(&qop, input_path, node->str, scratch), LOG_ID, "failed packing %.*s", str8_spread(node->str));
	}

	total_size = qop.archive_size + QOP_HEADER_SIZE;
	for(i = 0; i < qop.file_count; ++i) {
		qop_u64w(qop.files[i].hash, qop.file);
		qop_u32w((u32)qop.files[i].offset, qop.file);
		qop_u32w((u32)qop.files[i].size, qop.file);
		qop_u16w(qop.files[i].path_len, qop.file);
		qop_u16w(qop.files[i].flags, qop.file);
		total_size += QOP_INDEX_SIZE;
	}

	qop_u32w((u32)qop.file_count, qop.file);
	qop_u32w((u32)total_size, qop.file);
	qop_u32w(QOP_MAGIC, qop.file);

	res = true;
	log_info(LOG_ID, "files: %d, size: %_$$u", qop.file_count, (u32)total_size);

error:;
	if(sys_file_is_valid(qop.file)) {
		sys_file_close(qop.file);
	}
	return res;
}

int
main(int argc, char *argv[])
{
	int res                      = EXIT_FAILURE;
	struct alloc alloc_sys       = sys_allocator();
	struct alloc alloc           = {0};
	struct alloc scratch         = {0};
	struct marena marena         = {0};
	struct marena scratch_marena = {0};
	str8 in_path;
	str8 out_path;

	{
		usize mem_size = MMEGABYTE(4);
		void *mem      = mem_alloc_size(alloc_sys, mem_size);
		dbg_check_warn(mem, LOG_ID, "Failed to get scratch memory");
		marena_init(&scratch_marena, mem, mem_size);
	}
	{
		usize mem_size = MMEGABYTE(4);
		void *mem      = mem_alloc_size(alloc_sys, mem_size);
		dbg_check_warn(mem, LOG_ID, "Failed to get arena memory");
		marena_init(&marena, mem, mem_size);
	}

	scratch = marena_allocator(&scratch_marena);
	alloc   = marena_allocator(&marena);

	struct cmd_line cmd = cmd_line_from_argcv(scratch, argc, argv);

	if(cmd.inputs.node_count < 1) {
		sys_printf(
			"Usage: %.*s <input-folder|input-qop> [output-folder|output-qop]\n"
			"  output defaults to %s\n"
			"  example: %.*s ./tmp/assets/files ./tmp/assets.qop\n",
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
	dbg_check(qop_pack(alloc, in_path, out_path, scratch), LOG_ID, "pack failed");

	res = EXIT_SUCCESS;

error:;
	if(scratch_marena.buf) {
		sys_free(scratch_marena.buf);
	}
	if(marena.buf) {
		sys_free(marena.buf);
	}

	return res;
}
