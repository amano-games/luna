#include "sys-io.h"
#include "lib/dbg.h"
#include "sys-log.h"

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

struct sys_full_file_res
sys_load_full_file(str8 path, struct alloc alloc)
{
	struct sys_full_file_res res = {0};
	void *f                      = sys_file_open_r(path);

	dbg_check_warn(f != NULL, "io", "Failed to open file %*.s", (int)path.size, path.str);

	// Get file size
	sys_file_seek_end(f, 0);
	usize size = sys_file_tell(f);
	sys_file_seek_set(f, 0);

	// Alloc memory
	void *data = alloc.allocf(alloc.ctx, size);
	dbg_check(data != NULL, "io", "Failed alloc mem for: %*.s", (int)path.size, path.str);

	// Read contents
	sys_file_r(f, data, size);
	sys_file_close(f);

	res.data = data;
	res.size = size;

	log_info("sys", "Loaded full file contents %s %$$u", path.str, (uint)res.size);

	return res;

error:
	if(f != NULL) { sys_file_close(f); }
	return (struct sys_full_file_res){0};
}

// usize
// sys_file_modified(str8 path)
// {
// 	usize res = 0;
//
// #if !defined(TARGET_PLAYDATE)
// 	cf_time_t timestamp;
// 	cf_get_file_time((char *)path.str, &timestamp);
// 	res = timestamp.time;
// #endif
//
// 	return res;
// }
