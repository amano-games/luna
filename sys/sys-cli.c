#include "sys-cli.h"

#include <stdio.h>
#include <whereami.h>

#include "sys-assert.h"
#include "sys-io.h"
#include "sys-log.h"
#include "sys.h"

#define SOKOL_LOG_IMPL
#include "external/sokol/sokol_log.h"

void
sys_log(const char *tag, u32 log_level, u32 log_item, const char *msg, u32 line_nr, const char *filename)
{
	if(log_level <= SYS_LOG_LEVEL) {
		slog_func(tag, log_level, log_item, msg, line_nr, filename, NULL);
	}
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
	void *res = (void *)fopen((char *)path.str, "w");
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

i32
sys_file_w(void *f, const void *buf, u32 buf_size)
{
	int count = 1;
	usize s   = fwrite(buf, buf_size, count, (FILE *)f);
	return (i32)s;
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

bool32
sys_file_del(str8 path)
{
	return 0;
}
