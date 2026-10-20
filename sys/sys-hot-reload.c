#include "sys/sys-hot-reload.h"
#include "base/marena.h"
#include "base/str.h"
#include "sys/sys-io.h"
#include "sys/sys-log.h"

#define SYS_CODE_LABEL          "sys-hot"
#define SYS_CODE_TMP_COUNT      1024
#define SYS_CODE_COPY_ATTEMPTS  128
#define SYS_CODE_RELOAD_TRIES   100
#define SYS_CODE_RELOAD_WAIT_MS 100

static str8 sys_code_tmp_path(struct alloc alloc, struct sys_loaded_code *c, u32 idx);

struct sys_loaded_code
sys_code_ini(struct alloc alloc, str8 base_dir)
{
	struct sys_loaded_code res = {0};
	res.so.path                = str8_fmt_push(alloc, "%.*s/%s", str8_spread(base_dir), SYS_HOT_LIB_PATH);
	res.lock_path              = str8_fmt_push(alloc, "%.*s/%s", str8_spread(base_dir), SYS_HOT_LOCK_PATH);
	res.tmp_prefix             = str8_fmt_push(alloc, "%.*s/%s", str8_spread(base_dir), SYS_HOT_TMP_PREFIX);

	marena_stack(scratch_marena, MKILOBYTE(4));
	struct alloc scratch = marena_allocator(&scratch_marena);
	for(u32 idx = 0; idx < SYS_CODE_TMP_COUNT; ++idx) {
		marena_reset(&scratch_marena);
		sys_file_del(sys_code_tmp_path(scratch, &res, idx));
	}
	return res;
}

static app_void_fn *
sys_code_load_app_proc(sys_lib lib, enum app_proc proc)
{
	static const str8 proc_names[APP_PROC_NUM_COUNT] = {
		[APP_PROC_INIT]         = str8_lit_comp("app_init"),
		[APP_PROC_TICK]         = str8_lit_comp("app_tick"),
		[APP_PROC_DRAW]         = str8_lit_comp("app_draw"),
		[APP_PROC_AUDIO]        = str8_lit_comp("app_audio"),
		[APP_PROC_CLOSE]        = str8_lit_comp("app_close"),
		[APP_PROC_PAUSE]        = str8_lit_comp("app_pause"),
		[APP_PROC_RESUME]       = str8_lit_comp("app_resume"),
		[APP_PROC_STREAM_START] = str8_lit_comp("app_stream_start"),
		[APP_PROC_STREAM_END]   = str8_lit_comp("app_stream_end"),
	};
	str8 name = proc_names[proc];
	if(name.size == 0) {
		log_error(SYS_CODE_LABEL, "no symbol name for %.*s", str8_spread(APP_PROC_LABELS[proc]));
		return NULL;
	}
	app_void_fn *res = sys_lib_load_proc(lib, name);
	if(!res) { log_error(SYS_CODE_LABEL, "missing symbol %.*s", str8_spread(name)); }
	return res;
}

struct app_code
sys_code_load(struct sys_loaded_code *c)
{
	struct app_code res = {0};
	sys_lib lib         = {0};
	str8 tmp_path       = {0};
	b32 copied          = false;
	b32 ok              = true;

	marena_stack(scratch_marena, MKILOBYTE(4));
	struct alloc scratch = marena_allocator(&scratch_marena);

	if(sys_file_exists(c->lock_path, SYS_FILE_MODE_R)) { goto error; }

	c->so = sys_file_watch_ini(c->so.path);
	if(c->so.props.modified == 0) { goto error; }

	// The OS loader caches by path, every load needs a new one
	for(u32 attempt = 0; attempt < SYS_CODE_COPY_ATTEMPTS && !copied; ++attempt) {
		marena_reset(&scratch_marena);
		tmp_path   = sys_code_tmp_path(scratch, c, c->tmp_idx);
		c->tmp_idx = (c->tmp_idx + 1) % SYS_CODE_TMP_COUNT;

		// Delete instead of overwrite: on POSIX an older copy that is still mapped keeps its inode,
		// on Windows a loaded copy can't be deleted, the copy fails and the next index is tried
		sys_file_del(tmp_path);
		copied = sys_file_copy(c->so.path, tmp_path);
	}

	if(!copied) {
		log_error(SYS_CODE_LABEL, "failed to copy %.*s", str8_spread(c->so.path));
		goto error;
	}

	lib = sys_lib_open(tmp_path);
	if(!sys_lib_is_valid(lib)) { goto error; }

	for(enum app_proc proc = APP_PROC_NONE + 1; proc < APP_PROC_NUM_COUNT; ++proc) {
		res.procs[proc] = sys_code_load_app_proc(lib, proc);
		ok              = ok && res.procs[proc];
	}

	if(!ok) {
		sys_lib_close(lib);
		goto error;
	}

	res.is_valid = true;
	c->lib       = lib;
	log_info(SYS_CODE_LABEL, "loaded %.*s", str8_spread(tmp_path));
	return res;

error:
	mclr_struct(&res);
	return res;
}

void
sys_code_unload(struct sys_loaded_code *c, struct app_code *code)
{
	if(sys_lib_is_valid(c->lib)) { log_info(SYS_CODE_LABEL, "unloaded, library still mapped"); }
	c->lib = (sys_lib){0};
	mclr_struct(code);
}

b32
sys_code_reload(struct sys_loaded_code *c, struct app_code *code)
{
	sys_code_unload(c, code);
	for(u32 try_idx = 0; try_idx < SYS_CODE_RELOAD_TRIES; ++try_idx) {
		if(sys_file_exists(c->lock_path, SYS_FILE_MODE_R)) {
			sys_sleep_ms(SYS_CODE_RELOAD_WAIT_MS);
			continue;
		}
		*code = sys_code_load(c);
		break;
	}
	if(!code->is_valid) { log_error(SYS_CODE_LABEL, "reload failed, running without game code"); }
	return code->is_valid;
}

static str8
sys_code_tmp_path(struct alloc alloc, struct sys_loaded_code *c, u32 idx)
{
	str8 so_name = str8_skip_last_slash(c->so.path);
	str8 so_ext  = str8_skip(so_name, str8_chop_last_dot(so_name).size);
	return str8_fmt_push(alloc, "%.*s%u%.*s", str8_spread(c->tmp_prefix), idx, str8_spread(so_ext));
}
