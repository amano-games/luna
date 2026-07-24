#include "base/log.h"
#include "base/marena.h"
#include "base/mem.h"
#include "base/path.h"
#include "base/str.h"
#include "sys/sys-defs.h"
#include "sys/sys-os.h"

#include <direct.h>
#include <stdlib.h>

#include "whereami.h"

void
sys_os_boot_env(str8 exe_path, struct alloc scratch)
{
	(void)exe_path;
	(void)scratch;
}

void
sys_os_process_info_fill(struct sys_process_info *info, struct alloc alloc, struct alloc scratch)
{
	{
		ssize str_size = wai_getExecutablePath(NULL, 0, NULL);
		if(str_size > 0) {
			u8 *path = (u8 *)alloc_arr(scratch, path, str_size);
			wai_getExecutablePath((char *)path, str_size, NULL);
			info->exe_path = str8_cpy_push(alloc, (str8){.str = (u8 *)path, .size = str_size});
		}
	}
	{
		ssize str_size = wai_getModulePath(NULL, 0, NULL);
		if(str_size > 0) {
			u8 *path = alloc_arr(scratch, path, str_size);
			wai_getModulePath((char *)path, str_size, NULL);
			info->module_path = str8_cpy_push(alloc, (str8){.str = (u8 *)path, .size = str_size});
		}
	}

	info->initial_path = sys_os_get_current_path(alloc, scratch);

	{
		// TODO: SHGetFolderPathW / CSIDL_APPDATA (Win32 headers live here, not in Sokol helper)
		(void)scratch;
		info->data_path = str8_lit("");
	}
}

str8
sys_os_get_current_path(struct alloc alloc, struct alloc scratch)
{
	(void)alloc;
	(void)scratch;
	// TODO: GetCurrentDirectoryW
	return (str8){0};
}

b32
sys_os_make_dir(str8 path, struct alloc scratch)
{
	(void)scratch;
	return _mkdir((char *)path.str) == 0;
}

u32
sys_os_epoch_2000(u32 *milliseconds)
{
	// TODO: Win32 FILETIME / GetSystemTimeAsFileTime
	if(milliseconds) {
		*milliseconds = 0;
	}
	return 0;
}

#include "sys/sokol/sys-sokol-host.c"
