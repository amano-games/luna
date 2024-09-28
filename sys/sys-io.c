#include "sys-io.h"

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
