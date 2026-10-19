#include "sys/sys-hot-reload.h"
#include "base/marena.h"
#include "base/str.h"
#include "sys/sys-log.h"

#define SYS_CODE_LABEL         "hot-reload"
#define SYS_CODE_TEMP_COUNT    1024
#define SYS_CODE_COPY_ATTEMPTS 128

b32
sys_code_props_changed(struct sys_file_props loaded, struct sys_file_props current)
{
	if(current.modified == 0) { return false; }
	b32 res = current.modified != loaded.modified || current.size != loaded.size;
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

	str8 so_name = str8_skip_last_slash(c->so_path);
	str8 so_ext  = str8_skip(so_name, str8_chop_last_dot(so_name).size);

	if(sys_file_exists(c->lock_path, SYS_FILE_MODE_R)) { goto error; }

	c->props = sys_file_props_from_path(c->so_path);
	if(c->props.modified == 0) { goto error; }

	// The OS loader caches by path, every load needs a new one
	for(u32 attempt = 0; attempt < SYS_CODE_COPY_ATTEMPTS && !copied; ++attempt) {
		marena_reset(&scratch_marena);
		tmp_path   = str8_fmt_push(scratch, "%.*s%u%.*s", str8_spread(c->tmp_prefix), c->tmp_idx, str8_spread(so_ext));
		c->tmp_idx = (c->tmp_idx + 1) % SYS_CODE_TEMP_COUNT;

		// Delete instead of overwrite: on POSIX an older copy that is still mapped keeps its inode,
		// on Windows a loaded copy can't be deleted, the copy fails and the next index is tried
		sys_file_del(tmp_path);
		copied = sys_file_copy(c->so_path, tmp_path);
	}

	if(!copied) {
		log_error(SYS_CODE_LABEL, "failed to copy %.*s", str8_spread(c->so_path));
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
