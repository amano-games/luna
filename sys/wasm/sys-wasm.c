// @per_os_impl WASM platform — owns OS hooks; optional Sokol helper for present/window/audio.

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

#define SECONDS_BETWEEN_1970_AND_2000 946684800LL

void
sys_os_boot_env(str8 exe_path, struct alloc scratch)
{
	(void)exe_path;
	(void)scratch;
}

void
sys_os_process_info_fill(struct sys_process_info *info, struct alloc alloc, struct alloc scratch)
{
	(void)scratch;
	info->initial_path = sys_os_get_current_path(alloc, scratch);
	info->data_path    = str8_lit("");
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
