#include "sys/sys-hot-reload.h"

#if !(OS_LINUX || OS_MACOS)
#error sys-hot-reload.c is POSIX only
#endif

b32
sys_code_props_changed(struct sys_file_props loaded, struct sys_file_props current)
{
	if(current.modified == 0) { return false; }
	b32 res = current.modified != loaded.modified || current.size != loaded.size;
	return res;
}
