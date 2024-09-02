#include "sys-io.h"
#include "sys-backend.h"

sys_file *
sys_fopen(const char *path, const char *mode)
{
	switch(mode[0]) {
	case 'r': return (sys_file *)backend_file_open(path, SYS_FILE_R);
	case 'w': return (sys_file *)backend_file_open(path, SYS_FILE_W);
	}

	return NULL;
}

int
sys_fclose(sys_file *f)
{
	return backend_file_close(f);
}

usize
sys_fread(void *buf, usize size, usize count, sys_file *f)
{
	int i = backend_file_read(f, buf, size * count);
	return (i * count);
}

usize
sys_fwrite(const void *buf, usize size, usize count, sys_file *f)
{
	int i = backend_file_write(f, buf, size * count);
	return (i * count);
}

int
sys_ftell(sys_file *f)
{
	return backend_file_tell(f);
}

int
sys_fseek(sys_file *f, int pos, int origin)
{
	return backend_file_seek(f, pos, origin);
}

struct sys_file_stats
sys_fstats(const char *path)
{
	return backend_file_stats(path);
}
