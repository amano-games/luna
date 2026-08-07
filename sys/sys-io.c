#include "sys/sys-io.h"
#include "base/dbg.h"
#include "base/log.h"
#include "base/path.h"
#include "base/str.h"

void *
sys_file_open(str8 path, i32 sys_file_mode)
{
	switch(sys_file_mode) {
	case SYS_FILE_MODE_R: return sys_file_open_r(path);
	case SYS_FILE_MODE_W: return sys_file_open_w(path);
	case SYS_FILE_MODE_A: return sys_file_open_a(path);
	}
	return NULL;
}

b32
sys_file_exists(str8 path, i32 sys_file_mode)
{
	b32 res = false;
	void *f = sys_file_open(path, sys_file_mode);
	if(f != NULL) {
		res = true;
		sys_file_close(f);
	}
	return res;
}

struct sys_full_file_res
sys_load_full_file(struct alloc alloc, str8 path)
{
	struct sys_full_file_res res = {0};
	void *f                      = sys_file_open_r(path);

	dbg_check_warn(f != NULL, "io", "Failed to open file %.*s", str8_spread(path));

	// Get file size
	sys_file_seek_end(f, 0);
	usize f_size = sys_file_tell(f);
	sys_file_seek_set(f, 0);

	// Alloc memory
	void *data = alloc_size(alloc, f_size);
	dbg_check(data != NULL, "io", "Failed alloc mem for: %.*s", str8_spread(path));

	// Read contents
	sys_file_r(f, data, f_size);
	sys_file_close(f);

	res.data = data;
	res.size = f_size;

	log_info("sys", "Loaded full file contents %.*s %$$u", str8_spread(path), (uint)res.size);

	return res;

error:
	if(f != NULL) { sys_file_close(f); }
	return (struct sys_full_file_res){0};
}

str8
sys_path_to_data_path(struct alloc alloc, struct str8 path, str8 org_name, str8 app_name)
{
	str8 res       = path;
	str8 data_path = sys_data_path();
	if(data_path.size == 0) { return str8_cpy_push(alloc, res); }

	enum path_style path_style = path_style_from_str8(path);
	struct str8_list path_list = {0};
	str8_list_push(alloc, &path_list, data_path);
	str8_list_push(alloc, &path_list, app_name);
	str8_list_push(alloc, &path_list, path);
	res = path_join_by_style(alloc, &path_list, path_style);

	return res;
}
