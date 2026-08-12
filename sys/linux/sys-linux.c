// @per_os_impl Linux — owns OS sys_* ; optional Sokol helper for present/window/audio.

#include "base/log.h"
#include "base/marena.h"
#include "base/mem.h"
#include "base/path.h"
#include "base/str.h"
#include "sys/sys-defs.h"
#include "sys/sys-io.h"
#include "sys/sys-mem.h"
#include "sys/sys-os.h"
#include "sys/sys.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define SOKOL_TIME_IMPL
#include "sokol/sokol_time.h"

#define SECONDS_BETWEEN_1970_AND_2000 946684800LL
#define OS_ARENA_SIZE                 MMEGABYTE(1)
#define OS_SCRATCH_SIZE               MKILOBYTE(64)

static const str8 STEAM_RUNTIME_RELATIVE_PATH = str8_lit_comp("steam-runtime");

static struct {
	struct marena arena;
	struct alloc alloc;
	struct marena scratch_arena;
	struct alloc scratch;
	struct sys_process_info process_info;
	u64 tick_start;
	u64 tick_elapsed;
} OS_STATE;

// NOLINTNEXTLINE(readability-identifier-naming)
extern char **environ;

static void
sys_linux_boot_env(void)
{
	if(getenv("STEAM_RUNTIME")) {
		return;
	}
	str8 binary_path = OS_STATE.process_info.binary_path;
	if(binary_path.size == 0) {
		return;
	}

	marena_reset(&OS_STATE.scratch_arena);
	struct alloc scratch       = OS_STATE.scratch;
	struct str8_list path_list = {0};
	str8_list_push(scratch, &path_list, binary_path);
	str8_list_push(scratch, &path_list, STEAM_RUNTIME_RELATIVE_PATH);
	str8 runtime_path = path_join_by_style(scratch, &path_list, path_style_absolute_unix);
	log_info("SYS", "STEAM_RUNTIME %s", runtime_path.str);
	setenv("STEAM_RUNTIME", (char *)runtime_path.str, 1);
}

str8
sys_get_current_path(struct alloc alloc)
{
	str8 res    = {0};
	char *cwdir = getcwd(0, 0);
	if(cwdir) {
		res = str8_cpy_push(alloc, str8_cstr(cwdir));
		free(cwdir);
	}
	return res;
}

void
sys_os_init(void)
{
	struct alloc alloc_sys = sys_allocator();
	{
		void *mem = mem_alloc_size(alloc_sys, OS_ARENA_SIZE);
		marena_init(&OS_STATE.arena, mem, OS_ARENA_SIZE);
		OS_STATE.alloc = marena_allocator(&OS_STATE.arena);
	}
	{
		void *mem = mem_alloc_size(alloc_sys, OS_SCRATCH_SIZE);
		marena_init(&OS_STATE.scratch_arena, mem, OS_SCRATCH_SIZE);
		OS_STATE.scratch = marena_allocator(&OS_STATE.scratch_arena);
	}

	struct alloc alloc            = OS_STATE.alloc;
	struct alloc scratch          = OS_STATE.scratch;
	struct sys_process_info *info = &OS_STATE.process_info;
	*info                         = (struct sys_process_info){0};
	info->pid                     = (u32)getpid();

	{
		b32 got    = false;
		u8 *buffer = 0;
		i32 size   = 0;
		for(ssize cap = PATH_MAX, r = 0; r < 4; cap *= 2, r += 1) {
			marena_reset(&OS_STATE.scratch_arena);
			buffer = alloc_arr(scratch, buffer, cap);
			size   = (i32)readlink("/proc/self/exe", (char *)buffer, (size_t)cap);
			if(size > 0 && size < cap) {
				got = true;
				break;
			}
		}
		if(got && size > 0) {
			info->binary_file_path = str8_cpy_push(alloc, (str8){.str = buffer, .size = (usize)size});
			info->binary_path      = str8_chop_last_slash(info->binary_file_path);
		}
		marena_reset(&OS_STATE.scratch_arena);
	}

	info->initial_path = sys_get_current_path(alloc);

	{
		char *home            = getenv("HOME");
		char *xdg_config_home = getenv("XDG_CONFIG_HOME");
		char *xdg_cache_home  = getenv("XDG_CACHE_HOME");
		char *xdg_state_home  = getenv("XDG_STATE_HOME");
		if(xdg_config_home != NULL) {
			info->user_program_config_data_path = str8_cpy_push(alloc, str8_cstr(xdg_config_home));
		} else if(home != NULL) {
			info->user_program_config_data_path = str8_fmt_push(alloc, "%s/.config", home);
		}
		if(xdg_cache_home != NULL) {
			info->user_program_cache_data_path = str8_cpy_push(alloc, str8_cstr(xdg_cache_home));
		} else if(home != NULL) {
			info->user_program_cache_data_path = str8_fmt_push(alloc, "%s/.cache", home);
		}
		if(xdg_state_home != NULL) {
			info->user_program_logs_data_path = str8_cpy_push(alloc, str8_cstr(xdg_state_home));
		} else if(home != NULL) {
			info->user_program_logs_data_path = str8_fmt_push(alloc, "%s/.local/state", home);
		}
	}

	{
		for(char **e = environ; e && *e; ++e) {
			str8_list_push(alloc, &info->environment, str8_cpy_push(alloc, str8_cstr(*e)));
		}
	}

	sys_linux_boot_env();

	stm_setup();
	OS_STATE.tick_start   = stm_now();
	OS_STATE.tick_elapsed = OS_STATE.tick_start;
}

struct sys_process_info *
sys_process_info(void)
{
	return &OS_STATE.process_info;
}

str8
sys_base_path(void)
{
	return OS_STATE.process_info.base_path;
}

str8
sys_exe_path(void)
{
	return OS_STATE.process_info.binary_file_path;
}

str8
sys_data_path(void)
{
	return OS_STATE.process_info.user_program_config_data_path;
}

b32
sys_make_dir(str8 path)
{
	return mkdir((char *)path.str, 0755) != -1;
}

u32
sys_epoch_2000(u32 *milliseconds)
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

f32
sys_time_elapsed(void)
{
	return stm_sec(stm_since(OS_STATE.tick_elapsed));
}

void
sys_time_elapsed_reset(void)
{
	OS_STATE.tick_elapsed = stm_now();
}

u32
sys_time_us(void)
{
	return (u32)(stm_us(stm_since(OS_STATE.tick_start)));
}

u32
sys_time_ms(void)
{
	return (u32)(stm_ms(stm_since(OS_STATE.tick_start)));
}

void *
sys_alloc_raw(ssize size)
{
	return malloc(size);
}

void
sys_free_raw(void *ptr)
{
	free(ptr);
}

void *
sys_alloc(void *ptr, ssize size, ssize align)
{
	return sys_alloc_aligned_raw(size, align, sys_alloc_raw);
}

void
sys_free(void *ptr)
{
	sys_free_aligned_raw(ptr, sys_free_raw);
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

static long
sys_linux_file_size_get(const str8 path)
{
	FILE *fp = sys_file_open_r(path);

	if(fp == NULL)
		return -1;

	if(fseek(fp, 0, SEEK_END) < 0) {
		fclose(fp);
		return -1;
	}

	long size = sys_file_tell(fp);
	fclose(fp);
	return size;
}

struct sys_file_stats
sys_file_stats(str8 path)
{
	struct sys_file_stats res = {0};
	int size                  = (int)sys_linux_file_size_get(path);
	if(size < 0) {
		log_error("IO", "failed to get file stats %s", path.str);
	}
	res.size = size;
	return res;
}

void *
sys_file_open_r(const str8 path)
{
	return (void *)fopen((char *)path.str, "rb");
}

void *
sys_file_open_w(const str8 path)
{
	return (void *)fopen((char *)path.str, "wb");
}

void *
sys_file_open_a(const str8 path)
{
	return (void *)fopen((char *)path.str, "ab");
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

ssize
sys_file_w(void *f, const void *buf, u32 buf_size)
{
	i32 count = 1;
	ssize res = fwrite(buf, buf_size, count, (FILE *)f);
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
	return remove((char *)path.str) == 0;
}

b32
sys_file_rename(str8 from, str8 to)
{
	return (rename((char *)from.str, (char *)to.str) == 0);
}

usize
sys_file_modified(str8 path)
{
	return 0;
}

void
sys_set_auto_lock_disabled(int disable)
{
}

#if SYS_GFX_SOKOL
#include "sys/sokol/sys-sokol-host.c"
#endif
