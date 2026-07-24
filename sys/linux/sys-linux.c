#include "base/log.h"
#include "base/marena.h"
#include "base/mem.h"
#include "base/path.h"
#include "base/str.h"
#include "sys/sys-defs.h"
#include "sys/sys-os.h"

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "whereami.h"

#define SECONDS_BETWEEN_1970_AND_2000 946684800LL

static const str8 STEAM_RUNTIME_RELATIVE_PATH = str8_lit_comp("steam-runtime");

void
sys_os_boot_env(str8 exe_path, struct alloc scratch)
{
	if(getenv("STEAM_RUNTIME")) {
		return;
	}
	if(exe_path.size == 0) {
		return;
	}

	struct str8_list path_list = {0};
	str8_list_push(scratch, &path_list, exe_path);
	str8_list_push(scratch, &path_list, STEAM_RUNTIME_RELATIVE_PATH);
	str8 runtime_path = path_join_by_style(scratch, &path_list, path_style_absolute_unix);
	log_info("SYS", "STEAM_RUNTIME %s", runtime_path.str);
	setenv("STEAM_RUNTIME", (char *)runtime_path.str, 1);
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
		char *xdg      = getenv("XDG_DATA_HOME");
		char *home     = getenv("HOME");
		str8 data_path = str8_lit("");
		if(xdg != NULL) {
			data_path = str8_cstr(xdg);
		} else if(home != NULL) {
			data_path = str8_cstr(home);
		}
		info->data_path = str8_cpy_push(alloc, data_path);
	}
}

str8
sys_os_get_current_path(struct alloc alloc, struct alloc scratch)
{
	(void)scratch;
	str8 res    = {0};
	char *cwdir = getcwd(0, 0);
	if(cwdir) {
		res = str8_cpy_push(alloc, str8_cstr(cwdir));
		free(cwdir);
	}
	return res;
}

b32
sys_os_make_dir(str8 path, struct alloc scratch)
{
	str8 path_copy = str8_cpy_push(scratch, path);
	return mkdir((char *)path_copy.str, 0755) != -1;
}

u32
sys_os_epoch_2000(u32 *milliseconds)
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);

	u64 unix_seconds = (u64)ts.tv_sec;
	u64 seconds      = unix_seconds - SECONDS_BETWEEN_1970_AND_2000;

	if(milliseconds) {
		*milliseconds = (u32)(ts.tv_nsec / 1000000ULL);
	}

	return (u32)seconds;
}

#include "sys/sokol/sys-sokol-host.c"
