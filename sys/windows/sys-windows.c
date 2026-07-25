// @per_os_impl Windows — owns OS sys_* ; optional Sokol helper for present/window/audio.

#include "base/log.h"
#include "base/marena.h"
#include "base/mem.h"
#include "base/path.h"
#include "base/str.h"
#include "sys/sys-defs.h"
#include "sys/sys-io.h"
#include "sys/sys-os.h"
#include "sys/sys.h"

#include <direct.h>
#include <stdio.h>
#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>

#define SOKOL_TIME_IMPL
#include "sokol/sokol_time.h"

#define OS_ARENA_SIZE   MMEGABYTE(1)
#define OS_SCRATCH_SIZE MKILOBYTE(64)

static struct {
	struct marena arena;
	struct alloc alloc;
	struct marena scratch_arena;
	struct alloc scratch;
	struct sys_process_info process_info;
	u64 tick_start;
	u64 tick_elapsed;
} OS_STATE;

static str8
sys_windows_str8_from_wide(struct alloc alloc, const WCHAR *wide)
{
	str8 res = {0};
	if(wide == NULL) {
		return res;
	}
	i32 bytes = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);
	if(bytes <= 1) {
		return res;
	}
	u8 *buf = alloc_arr(alloc, buf, bytes);
	WideCharToMultiByte(CP_UTF8, 0, wide, -1, (char *)buf, bytes, NULL, NULL);
	res = (str8){.str = buf, .size = (usize)(bytes - 1)};
	return res;
}

str8
sys_get_current_path(struct alloc alloc)
{
	DWORD needed = GetCurrentDirectoryW(0, NULL);
	if(needed == 0) {
		return (str8){0};
	}
	marena_reset(&OS_STATE.scratch_arena);
	WCHAR *wide = alloc_arr(OS_STATE.scratch, wide, needed);
	if(GetCurrentDirectoryW(needed, wide) == 0) {
		marena_reset(&OS_STATE.scratch_arena);
		return (str8){0};
	}
	str8 res = sys_windows_str8_from_wide(alloc, wide);
	marena_reset(&OS_STATE.scratch_arena);
	return res;
}

void
sys_os_init(void)
{
	{
		void *mem = sys_alloc(NULL, OS_ARENA_SIZE, 8);
		marena_init(&OS_STATE.arena, mem, OS_ARENA_SIZE);
		OS_STATE.alloc = marena_allocator(&OS_STATE.arena);
	}
	{
		void *mem = sys_alloc(NULL, OS_SCRATCH_SIZE, 8);
		marena_init(&OS_STATE.scratch_arena, mem, OS_SCRATCH_SIZE);
		OS_STATE.scratch = marena_allocator(&OS_STATE.scratch_arena);
	}

	struct alloc alloc            = OS_STATE.alloc;
	struct alloc scratch          = OS_STATE.scratch;
	struct sys_process_info *info = &OS_STATE.process_info;
	*info                         = (struct sys_process_info){0};
	info->pid                     = (u32)GetCurrentProcessId();

	{
		DWORD size = 32 * 1024;
		marena_reset(&OS_STATE.scratch_arena);
		WCHAR *buffer = alloc_arr(scratch, buffer, size);
		DWORD length  = GetModuleFileNameW(0, buffer, size);
		if(length > 0 && length < size) {
			// UTF-8 into alloc: scratch is already full of WCHARs (size * sizeof(WCHAR)).
			info->binary_file_path = sys_windows_str8_from_wide(alloc, buffer);
			info->binary_path      = str8_chop_last_slash(info->binary_file_path);
		}
		marena_reset(&OS_STATE.scratch_arena);
	}

	info->initial_path = sys_get_current_path(alloc);

	{
		marena_reset(&OS_STATE.scratch_arena);
		WCHAR *buffer = alloc_arr(scratch, buffer, MAX_PATH);
		if(SUCCEEDED(SHGetFolderPathW(0, CSIDL_APPDATA, 0, 0, buffer))) {
			str8 appdata                      = sys_windows_str8_from_wide(alloc, buffer);
			info->user_program_config_data_path = appdata;
			info->user_program_cache_data_path  = appdata;
			info->user_program_logs_data_path   = appdata;
		}
		marena_reset(&OS_STATE.scratch_arena);
	}

	{
		WCHAR *env = GetEnvironmentStringsW();
		if(env) {
			usize start_idx = 0;
			for(usize idx = 0;; idx += 1) {
				if(env[idx] == 0) {
					if(start_idx == idx) {
						break;
					}
					str8 entry = sys_windows_str8_from_wide(alloc, env + start_idx);
					str8_list_push(alloc, &info->environment, entry);
					start_idx = idx + 1;
				}
			}
			FreeEnvironmentStringsW(env);
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
	return _mkdir((char *)path.str) == 0;
}

u32
sys_epoch_2000(u32 *milliseconds)
{
	// TODO: Win32 FILETIME / GetSystemTimeAsFileTime
	if(milliseconds) {
		*milliseconds = 0;
	}
	return 0;
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

void *
sys_alloc(void *ptr, ssize size, ssize align)
{
	void *res = malloc(size);
	dbg_check(res, "sys-windows", "Alloc failed to get %" PRIu32 ", %$$u", size, (uint)size);

error:
	return res;
}

void
sys_free(void *ptr)
{
	free(ptr);
}

static long
sys_windows_file_size_get(const str8 path)
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
	int size                  = (int)sys_windows_file_size_get(path);
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
