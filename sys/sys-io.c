#include "sys/sys-io.h"
#include "base/dbg.h"
#include "sys/sys-log.h"
#include "base/path.h"
#include "base/str.h"
#include "sys/sys-os.h"

#define SYS_FILE_WATCH_POLL_US (200 * 1000)

b32
sys_make_dirs(str8 path)
{
	if(path.size == 0) { return true; }
	if(!path.str || path.size >= PTRDIFF_MAX || str8_find_needle(path, 0, str8_lit("\0"), 0) < path.size) { return false; }

	b32 ok = false;
	for(u64 slash_pos = 0; slash_pos <= path.size;
		slash_pos     = str8_find_needle(path, slash_pos + 1, str8_lit("/"), str_match_flag_slash_insensitive)) {
		str8 ancestor = str8_prefix(path, slash_pos);
		if(ancestor.size != 0) {
			ok = sys_make_dir(ancestor);
		}
		if(slash_pos == path.size) { break; }
	}

	return ok;
}

sys_file
sys_file_open(str8 path, i32 sys_file_mode)
{
	switch(sys_file_mode) {
	case SYS_FILE_MODE_R: return sys_file_open_r(path);
	case SYS_FILE_MODE_W: return sys_file_open_w(path);
	case SYS_FILE_MODE_A: return sys_file_open_a(path);
	}
	return sys_file_zero();
}

b32
sys_file_exists(str8 path, i32 sys_file_mode)
{
	b32 res    = false;
	sys_file f = sys_file_open(path, sys_file_mode);
	if(sys_file_is_valid(f)) {
		res = true;
		sys_file_close(f);
	}
	return res;
}

// Fallback for sys_file_copy
// IDK if I like this or should be copied over on wasm and pd
static b32
sys_file_copy_chunked(str8 from, str8 to)
{
	b32 res      = false;
	sys_file in  = sys_file_open_r(from);
	sys_file out = sys_file_is_valid(in) ? sys_file_open_w(to) : sys_file_zero();
	if(sys_file_is_valid(in) && sys_file_is_valid(out)) {
		u8 buf[MKILOBYTE(4)];
		res = true;
		for(;;) {
			ssize n = sys_file_r(in, buf, sizeof(buf));
			if(n < 0 || (n > 0 && sys_file_w(out, buf, (u32)n) != n)) {
				res = false;
				break;
			}
			if(n == 0) { break; }
		}
	}
	if(sys_file_is_valid(in)) { sys_file_close(in); }
	if(sys_file_is_valid(out)) {
		res = sys_file_close(out) && res;
		if(!res) { sys_file_del(to); }
	}
	return res;
}

struct sys_full_file_res
sys_load_full_file(struct alloc alloc, str8 path)
{
	struct sys_full_file_res res = {0};
	sys_file f                   = sys_file_open_r(path);

	dbg_check_warn(sys_file_is_valid(f), "io", "Failed to open file %.*s", str8_spread(path));

	// Get file size
	sys_file_seek_end(f, 0);
	usize f_size = (usize)sys_file_tell(f);
	sys_file_seek_set(f, 0);

	// Alloc memory
	void *data = mem_alloc_size(alloc, f_size);
	dbg_check(data != NULL, "io", "Failed alloc mem for: %.*s", str8_spread(path));

	// Read contents
	sys_file_r(f, data, (u32)f_size);
	sys_file_close(f);

	res.data = data;
	res.size = f_size;

	char size_b[STR_BYTES_CAP];
	log_info("sys", "Loaded full file contents %.*s %.*s", str8_spread(path), str8_spread(str8_from_bytes_buf(size_b, sizeof(size_b), (u64)res.size)));

	return res;

error:
	if(sys_file_is_valid(f)) { sys_file_close(f); }
	return (struct sys_full_file_res){0};
}

str8
sys_config_path(void)
{
	return sys_process_info()->user_program_config_data_path;
}

str8
sys_data_path(void)
{
	return sys_process_info()->user_program_data_path;
}

str8
sys_cache_path(void)
{
	return sys_process_info()->user_program_cache_data_path;
}

str8
sys_logs_path(void)
{
	return sys_process_info()->user_program_logs_data_path;
}

// {root}/{app_name}/{path}; empty root → copy of path only.
static str8
sys_path_to_root(struct alloc alloc, str8 root, str8 path, str8 org_name, str8 app_name)
{
	if(root.size == 0) {
		return str8_cpy_push(alloc, path);
	}

	enum path_style path_style = path_style_from_str8(path);
	struct str8_list path_list = {0};
	str8_list_push(alloc, &path_list, root);
	str8_list_push(alloc, &path_list, app_name);
	str8_list_push(alloc, &path_list, path);
	return path_join_by_style(alloc, &path_list, path_style);
}

str8
sys_path_to_config_path(struct alloc alloc, str8 path, str8 org_name, str8 app_name)
{
	return sys_path_to_root(alloc, sys_config_path(), path, org_name, app_name);
}

str8
sys_path_to_data_path(struct alloc alloc, str8 path, str8 org_name, str8 app_name)
{
	return sys_path_to_root(alloc, sys_data_path(), path, org_name, app_name);
}

str8
sys_path_to_cache_path(struct alloc alloc, str8 path, str8 org_name, str8 app_name)
{
	return sys_path_to_root(alloc, sys_cache_path(), path, org_name, app_name);
}

str8
sys_path_to_log_path(struct alloc alloc, str8 path, str8 org_name, str8 app_name)
{
	return sys_path_to_root(alloc, sys_logs_path(), path, org_name, app_name);
}

struct sys_file_watch
sys_file_watch_ini(str8 path)
{
	struct sys_file_watch res = {0};
	res.path                  = path;
	res.props                 = sys_file_props_from_path(path);
	return res;
}

b32
sys_file_watch_changed(struct sys_file_watch *w, b32 throttle)
{
	u32 now = sys_time_us();

	// NOTE: u32 wrap (~71 min)
	if(throttle && now - w->poll_last_us < SYS_FILE_WATCH_POLL_US) { return false; }

	w->poll_last_us = now;

	if(w->path.size == 0) { return false; }

	if(!sys_file_exists(w->path, SYS_FILE_MODE_R)) { return false; }

	struct sys_file_props current = sys_file_props_from_path(w->path);

	if(current.modified == 0) { return false; }

	b32 res = current.modified != w->props.modified || current.size != w->props.size;

	return res;
}
