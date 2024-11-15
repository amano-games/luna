#include "sys-io.h"
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

	if(f == NULL) {
		log_error("io", "failed to open file: %s", path.str);
		return res;
	}

	sys_file_seek_end(f, 0);
	usize size = sys_file_tell(f);
	sys_file_seek_set(f, 0);
	void *data = alloc.allocf(alloc.ctx, size);

	if(!data) {
		sys_file_close(f);
		log_error("io", "failed to load %s", path.str);
		return res;
	}

	sys_file_r(f, data, size);
	sys_file_close(f);

	res.data = data;
	res.size = size;

	return res;
}
