// @per_os_impl macOS — owns OS sys_* ; optional Sokol helper for present/window/audio.

#include "base/log.h"
#include "base/marena.h"
#include "base/mem.h"
#include "base/path.h"
#include "base/str.h"
#include "sys/sys-defs.h"
#include "sys/sys-io.h"
#include "sys/sys-os.h"
#include "sys/sys.h"

#include <mach-o/dyld.h>
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
	{
		// TODO: mem align
		void *mem = sys_alloc(NULL, OS_ARENA_SIZE, 8);
		marena_init(&OS_STATE.arena, mem, OS_ARENA_SIZE);
		OS_STATE.alloc = marena_allocator(&OS_STATE.arena);
	}
	{
		// TODO: mem align
		void *mem = sys_alloc(NULL, OS_SCRATCH_SIZE, 8);
		marena_init(&OS_STATE.scratch_arena, mem, OS_SCRATCH_SIZE);
		OS_STATE.scratch = marena_allocator(&OS_STATE.scratch_arena);
	}

	struct alloc alloc            = OS_STATE.alloc;
	struct alloc scratch          = OS_STATE.scratch;
	struct sys_process_info *info = &OS_STATE.process_info;
	*info                         = (struct sys_process_info){0};
	info->pid                     = (u32)getpid();

	{
		u32 size = 0;
		_NSGetExecutablePath(NULL, &size);
		if(size > 0) {
			marena_reset(&OS_STATE.scratch_arena);
			// TODO: mem align
			char *buf = alloc_arr(scratch, buf, size);
			if(_NSGetExecutablePath(buf, &size) == 0) {
				info->binary_file_path = str8_cpy_push(alloc, str8_cstr(buf));
				info->binary_path      = str8_chop_last_slash(info->binary_file_path);
			}
			marena_reset(&OS_STATE.scratch_arena);
		}
	}

	info->initial_path = sys_get_current_path(alloc);

	{
		char *home = getenv("HOME");
		if(home != NULL) {
			str8 home_s                         = str8_cstr(home);
			info->user_program_config_data_path = str8_cat_push(alloc, home_s, str8_lit("/Library/Application Support"));
			info->user_program_cache_data_path  = str8_cat_push(alloc, home_s, str8_lit("/Library/Caches"));
			info->user_program_logs_data_path   = str8_cat_push(alloc, home_s, str8_lit("/Library/Logs"));
		}
	}

	{
		str8 exe_path = info->binary_file_path;
		if(exe_path.size > 0) {
			str8 macos                 = str8_chop_last_slash(exe_path);
			str8 contents              = str8_chop_last_slash(macos);
			str8 resources_rel         = str8_lit("Resources");
			enum path_style path_style = path_style_from_str8(resources_rel);
			struct str8_list path_list = {0};
			marena_reset(&OS_STATE.scratch_arena);
			str8_list_push(scratch, &path_list, contents);
			str8_list_push(scratch, &path_list, resources_rel);
			info->base_path = path_join_by_style(alloc, &path_list, path_style);
			marena_reset(&OS_STATE.scratch_arena);
		}
	}

	{
		for(char **e = environ; e && *e; ++e) {
			str8_list_push(alloc, &info->environment, str8_cpy_push(alloc, str8_cstr(*e)));
		}
	}

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

struct alloc
sys_allocator(void)
{
	struct alloc alloc = {
		.allocf = sys_alloc,
		.ctx    = NULL,
	};
	return alloc;
}

// TODO: mem align
void *
sys_alloc(void *ptr, ssize size, ssize align)
{
	void *res = malloc(size);
	dbg_check(res, "sys-macos", "Alloc failed to get %" PRIu32 ", %$$u", size, (uint)size);

error:
	return res;
}

void
sys_free(void *ptr)
{
	free(ptr);
}

static long
sys_macos_file_size_get(const str8 path)
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
	int size                  = (int)sys_macos_file_size_get(path);
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

#include "sys/sokol/sys-sokol-host.c"
