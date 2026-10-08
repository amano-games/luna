#include "sys/sys-io.h"
#include "base/dbg.h"
#include "base/log.h"
#include "base/path.h"
#include "base/str.h"

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

	log_info("sys", "Loaded full file contents %.*s %$$u", str8_spread(path), (uint)res.size);

	return res;

error:
	if(sys_file_is_valid(f)) { sys_file_close(f); }
	return (struct sys_full_file_res){0};
}

// Pack 1-based FileStat-style calendar into dense_time (0-based day/mon).
dense_time
sys_file_modified(str8 path)
{
	struct sys_file_props props = sys_file_props_get(path);
	struct date_time dt         = {0};

	dt.year = (u32)props.m_year;
	dt.mon  = (u32)(props.m_month > 0 ? props.m_month - 1 : 0);
	dt.day  = (u32)(props.m_day > 0 ? props.m_day - 1 : 0);
	dt.hour = (u16)props.m_hour;
	dt.min  = (u16)props.m_minute;
	dt.sec  = (u16)props.m_second;

	return dense_time_from_date_time(dt);
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
