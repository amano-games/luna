#pragma once

#include "base/date-time.h"
#include "base/mem.h"
#include "base/path.h"
#include "base/str.h"
#include "base/types.h"
#include "sys/sys.h"

enum {
	SYS_FILE_MODE_R,
	SYS_FILE_MODE_W,
	SYS_FILE_MODE_A,
};

enum {
	SYS_FILE_SEEK_SET, // SEEK_SET
	SYS_FILE_SEEK_CUR, // SEEK_CUR
	SYS_FILE_SEEK_END, // SEEK_END
};

enum sys_file_prop_flag {
	SYS_FILE_PROP_FLAG_IS_DIR = 1 << 0,
};

struct sys_file_props {
	u64 size;
	dense_time modified;
	dense_time created;
	u32 flags;
};

struct sys_file_watch {
	str8 path;
	struct sys_file_props props;
	u64 poll_last_us;
};

// Opaque OS file handle (FILE* / SDFile* stored in u64[0]).
typedef struct sys_file sys_file;
struct sys_file {
	u64 u64[1];
};

struct sys_full_file_res {
	void *data;
	usize size;
};

/*
 * Contract (all platforms):
 * - open_*: zero handle on failure
 * - r/w: byte counts; -1 hard error; 0 EOF / empty
 * - close/flush/del/rename/copy/mkdir: b32 true = success
 * - copy: overwrites to; a missing from never creates to; a partial to is deleted
 * - seek_*: 0 = success
 * - tell: byte offset (negative on error)
 * - props_from_*: zero props on failure; modified/created are UTC dense_time with msec (Playdate: seconds only)
 */

static inline sys_file
sys_file_zero(void)
{
	return (sys_file){0};
}

static inline b32
sys_file_is_valid(sys_file f)
{
	return f.u64[0] != 0;
}

sys_file sys_file_open(str8 path, i32 sys_file_mode);
b32 sys_file_exists(str8 path, i32 sys_file_mode);
struct sys_full_file_res sys_load_full_file(struct alloc alloc, str8 path);
b32 sys_make_dirs(str8 path);

struct sys_file_watch sys_file_watch_ini(str8 path);
b32 sys_file_watch_changed(struct sys_file_watch *w, b32 throttle);

// @per_os_impl File IO
sys_file sys_file_open_r(str8 path);
sys_file sys_file_open_w(str8 path);
sys_file sys_file_open_a(str8 path);
b32 sys_file_close(sys_file f);
b32 sys_file_del(str8 path);
b32 sys_file_rename(str8 from, str8 to);
b32 sys_file_replace(str8 from, str8 to);
b32 sys_file_copy(str8 from, str8 to);
b32 sys_file_flush(sys_file f);
i32 sys_file_tell(sys_file f);
i32 sys_file_seek_set(sys_file f, i32 pos);
i32 sys_file_seek_cur(sys_file f, i32 pos);
i32 sys_file_seek_end(sys_file f, i32 pos);
ssize sys_file_w(sys_file f, const void *buf, u32 bsize);
ssize sys_file_r(sys_file f, void *buf, u32 bsize);
struct sys_file_props sys_file_props_from_file(sys_file f);
struct sys_file_props sys_file_props_from_path(str8 path);
b32 sys_make_dir(str8 path);

// @per_os_impl Paths
str8 sys_exe_path(void);
str8 sys_base_path(void);

str8 sys_config_path(void);
str8 sys_data_path(void);
str8 sys_cache_path(void);
str8 sys_logs_path(void);

str8 sys_path_to_config_path(struct alloc alloc, str8 path, str8 org_name, str8 app_name);
str8 sys_path_to_data_path(struct alloc alloc, str8 path, str8 org_name, str8 app_name);
str8 sys_path_to_cache_path(struct alloc alloc, str8 path, str8 org_name, str8 app_name);
str8 sys_path_to_log_path(struct alloc alloc, str8 path, str8 org_name, str8 app_name);

static inline str8
sys_path_timestamp(struct alloc alloc)
{
	struct date_time dt = date_time_from_epoch_2000_gmt(sys_epoch_2000(NULL));
	// 2026-10-13--19-14-03
	return str_from_date_time__file_name(alloc, &dt);
}
