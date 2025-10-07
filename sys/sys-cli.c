#include "sys-cli.h"

#include "base/str.h"
#include "sys-io.h"
#include "base/log.h"
#include "sys.h"
#include <sys/stat.h>

#define SOKOL_LOG_IMPL
#include "sokol/sokol_log.h"

struct cli_state {
	struct sys_process_info process_info;
};

static struct cli_state CLI_STATE;

void
sys_log(
	const char *tag,
	enum sys_log_level log_level,
	u32 log_item,
	const char *msg,
	u32 line_nr,
	const char *filename)
{
	if(log_level <= SYS_LOG_LEVEL) {
#if defined DEBUG
		const char *fn = filename;
#else
		const char *fn = NULL;
#endif
		slog_func(tag, log_level, log_item, msg, line_nr, fn, NULL);
	}
}

struct alloc
sys_allocator(void)
{
	struct alloc alloc = {
		.allocf = sys_alloc,
		.ctx    = NULL,
	};
	return alloc;
}

void *
sys_alloc(void *ptr, usize size)
{
	return malloc(size);
}

void
sys_free(void *ptr)
{
	free(ptr);
}

void *
sys_file_open_r(const str8 path)
{
	void *res = (void *)fopen((char *)path.str, "rb");
	return res;
}

void *
sys_file_open_w(const str8 path)
{
	void *res = (void *)fopen((char *)path.str, "wb");
	return res;
}

void *
sys_file_open_a(const str8 path)
{
	void *res = (void *)fopen((char *)path.str, "ab");
	return res;
}

i32
sys_file_close(void *f)
{
	return fclose((FILE *)f);
}

i32
sys_file_flush(void *f)
{
	return fflush((FILE *)f);
}

i32
sys_file_r(void *f, void *buf, u32 buf_size)
{
	i32 count = 1;
	usize s   = fread(buf, buf_size, count, (FILE *)f);
	if(s == 0) {
		log_error("IO", "Error reading from file: %d", (int)s);
	}

	return (i32)s;
}

size
sys_file_w(void *f, const void *buf, u32 buf_size)
{
	int count = 1;
	size res  = fwrite(buf, buf_size, count, (FILE *)f);
	return res;
}

i32
sys_file_tell(void *f)
{
	usize t = ftell((FILE *)f);
	return (i32)t;
}

i32
sys_file_seek_set(void *f, i32 pos)
{
	return (i32)fseek((FILE *)f, pos, SEEK_SET);
}

i32
sys_file_seek_cur(void *f, i32 pos)
{
	return (i32)fseek((FILE *)f, pos, SEEK_CUR);
}

i32
sys_file_seek_end(void *f, i32 pos)
{
	return (i32)fseek((FILE *)f, pos, SEEK_END);
}

b32
sys_file_del(str8 path)
{
	return 0;
}

str8
sys_base_path(void)
{
	str8 res = {0};
	return res;
}

str8
sys_data_path(void)
{
	str8 res = {0};
	return res;
}

str8
sys_exe_path(void)
{
	str8 res = {0};
	return res;
}

str8
sys_current_path(struct alloc alloc)
{
	str8 res = {0};
	return res;
}

struct sys_process_info
sys_process_info(void)
{
	struct sys_process_info res = {0};
	return res;
}

#if defined(TARGET_WIN)
#include <stdlib.h>
#endif
b32
sys_make_dir(str8 path)
{
	// TODO: OS Layer that doesn't depend on sokol
	b32 res = false;
#if defined(TARGET_WIN)
	{
		res = _mkdir(path) == 0;
	}
#else
	{
		str8 path_copy = str8_cpy_push(sys_allocator(), path);
		if(mkdir((char *)path_copy.str, 0755) != -1) {
			res = 1;
		}
		if(path_copy.str) {
			sys_free(path_copy.str);
		}
	}
#endif

	return res;
}
