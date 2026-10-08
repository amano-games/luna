// @per_os_impl Windows — owns OS sys_* ; optional Sokol helper for present/window/audio.

#include "sys/sys-log.h"
#include "base/marena.h"
#include "base/mem.h"
#include "base/path.h"
#include "base/str.h"
#include "sys/sys-defs.h"
#include "sys/sys-io.h"
#include "sys/sys-mem.h"
#include "sys/sys-log.h"
#include "sys/sys-os.h"
#include "sys/sys.h"

#if SYS_GFX
#include "sys/sys-gamepad.h"
#include "sys/sys-keyboard.h"
#endif

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

static SRWLOCK OS_AUDIO_LOCK = SRWLOCK_INIT;

str8 sys_get_current_path(struct alloc alloc);

void
sys_os_init(void)
{
	// Attach GUI builds to the launching shell, preserving redirected streams.
	HANDLE out_handle = GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE err_handle = GetStdHandle(STD_ERROR_HANDLE);
	if(AttachConsole(ATTACH_PARENT_PROCESS)) {
		if(!out_handle || out_handle == INVALID_HANDLE_VALUE) { (void)freopen("CONOUT$", "w", stdout); }
		if(!err_handle || err_handle == INVALID_HANDLE_VALUE) { (void)freopen("CONOUT$", "w", stderr); }
	}

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
	info->pid                     = (u32)GetCurrentProcessId();

	{
		DWORD size = 32 * 1024;
		marena_reset(&OS_STATE.scratch_arena);
		WCHAR *buffer = alloc_arr(scratch, buffer, size);
		DWORD length  = GetModuleFileNameW(0, buffer, size);
		if(length > 0 && length < size) {
			// UTF-8 into alloc: scratch is already full of WCHARs (size * sizeof(WCHAR)).
			info->binary_file_path = str8_from_16(alloc, str16_cstr((u16 *)buffer));
			info->binary_path      = str8_chop_last_slash(info->binary_file_path);
		}
		marena_reset(&OS_STATE.scratch_arena);
	}

	info->initial_path = sys_get_current_path(alloc);

	{
		marena_reset(&OS_STATE.scratch_arena);
		WCHAR *buffer = alloc_arr(scratch, buffer, MAX_PATH);

		if(SUCCEEDED(SHGetFolderPathW(0, CSIDL_APPDATA, 0, 0, buffer))) {
			str8 appdata                        = str8_from_16(alloc, str16_cstr((u16 *)buffer));
			info->user_program_config_data_path = appdata;
			info->user_program_data_path        = appdata;
		} else {
			log_warn("os", "SHGetFolderPathW(CSIDL_APPDATA) failed");
		}

		marena_reset(&OS_STATE.scratch_arena);
		buffer = alloc_arr(scratch, buffer, MAX_PATH);

		if(SUCCEEDED(SHGetFolderPathW(0, CSIDL_LOCAL_APPDATA, 0, 0, buffer))) {
			str8 local                         = str8_from_16(alloc, str16_cstr((u16 *)buffer));
			info->user_program_cache_data_path = local;
			info->user_program_logs_data_path  = local;
		} else {
			log_warn("os", "SHGetFolderPathW(CSIDL_LOCAL_APPDATA) failed");
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
					str8 entry = str8_from_16(alloc, (str16){(u16 *)(env + start_idx), idx - start_idx});
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

#if SYS_GFX
	sys_os_gamepad_ini();
	sys_os_keyboard_ini();
#endif
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

b32
sys_make_dir(str8 path)
{
	// TODO: use sys-scratch for the temporary path buffer.
	WCHAR name16[1024];
	dbg_assert(path.size < 1024);
	if(!path.str || path.size == 0 || path.size >= 1024) { return false; }

	b32 result = false;
	i32 n      = MultiByteToWideChar(CP_UTF8, 0, (char *)path.str, (int)path.size, name16, 1024 - 1);
	if(n <= 0) {
		return result;
	}
	name16[n] = 0;

	WIN32_FILE_ATTRIBUTE_DATA attributes = {0};
	GetFileAttributesExW(name16, GetFileExInfoStandard, &attributes);
	if(attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		result = true;
	} else if(CreateDirectoryW(name16, 0)) {
		result = true;
	}
	return result;
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
	return (u32)(u64)(stm_us(stm_since(OS_STATE.tick_start)));
}

u32
sys_time_ms(void)
{
	return (u32)(u64)(stm_ms(stm_since(OS_STATE.tick_start)));
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

static sys_file
sys_file_from_fp(FILE *fp)
{
	sys_file f = sys_file_zero();
	f.u64[0]   = (u64)(uptr)fp;
	return f;
}

static FILE *
sys_file_fp(sys_file f)
{
	return (FILE *)(uptr)f.u64[0];
}

struct sys_file_props
sys_file_props_get(str8 path)
{
	struct sys_file_props res           = {0};
	WIN32_FILE_ATTRIBUTE_DATA attr_data = {0};

	if(!GetFileAttributesExA((char *)path.str, GetFileExInfoStandard, &attr_data)) {
		log_error("IO", "failed to get file stats %s", path.str);
		return res;
	}

	res.size  = attr_data.nFileSizeLow;
	res.isdir = (attr_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0;

	// Fill 1-based calendar fields for shared dense_time packing.
	{
		SYSTEMTIME st = {0};
		FileTimeToSystemTime(&attr_data.ftLastWriteTime, &st);
		res.m_year   = st.wYear;
		res.m_month  = st.wMonth;
		res.m_day    = st.wDay;
		res.m_hour   = st.wHour;
		res.m_minute = st.wMinute;
		res.m_second = st.wSecond;
	}

	return res;
}

sys_file
sys_file_open_r(str8 path)
{
	return sys_file_from_fp(fopen((char *)path.str, "rb"));
}

sys_file
sys_file_open_w(str8 path)
{
	return sys_file_from_fp(fopen((char *)path.str, "wb"));
}

sys_file
sys_file_open_a(str8 path)
{
	return sys_file_from_fp(fopen((char *)path.str, "ab"));
}

b32
sys_file_close(sys_file f)
{
	if(!sys_file_is_valid(f)) { return false; }
	return fclose(sys_file_fp(f)) == 0;
}

b32
sys_file_flush(sys_file f)
{
	if(!sys_file_is_valid(f)) { return false; }
	return fflush(sys_file_fp(f)) == 0;
}

ssize
sys_file_r(sys_file f, void *buf, u32 buf_size)
{
	FILE *fp = sys_file_fp(f);
	usize s  = fread(buf, 1, buf_size, fp);
	if(s == 0 && ferror(fp)) {
		log_error("IO", "Error reading from file");
		return -1;
	}
	return (ssize)s;
}

ssize
sys_file_w(sys_file f, const void *buf, u32 buf_size)
{
	FILE *fp = sys_file_fp(f);
	usize s  = fwrite(buf, 1, buf_size, fp);
	if(s == 0 && buf_size > 0 && ferror(fp)) {
		return -1;
	}
	return (ssize)s;
}

i32
sys_file_tell(sys_file f)
{
	return (i32)ftell(sys_file_fp(f));
}

i32
sys_file_seek_set(sys_file f, i32 pos)
{
	return (i32)fseek(sys_file_fp(f), pos, SEEK_SET);
}

i32
sys_file_seek_cur(sys_file f, i32 pos)
{
	return (i32)fseek(sys_file_fp(f), pos, SEEK_CUR);
}

i32
sys_file_seek_end(sys_file f, i32 pos)
{
	return (i32)fseek(sys_file_fp(f), pos, SEEK_END);
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

b32
sys_file_replace(str8 from, str8 to)
{
	if(!from.size || !to.size || from.size > 32767 * 4 || to.size > 32767 * 4) { return false; }

	ssize scratch_size = (ssize)((from.size + to.size + 2) * sizeof(u16) + MEM_ALIGN_DEFAULT);
	marena_stack(arena, MKILOBYTE(4));
	if(scratch_size > marena_size_rem(&arena)) { return false; }
	struct alloc scratch = marena_allocator(&arena);
	str16 from_wide      = str16_from_8(scratch, from);
	str16 to_wide        = str16_from_8(scratch, to);
	if(!from_wide.str || !to_wide.str || from_wide.size > 32767 || to_wide.size > 32767) { return false; }
	return MoveFileExW((WCHAR *)from_wide.str, (WCHAR *)to_wide.str, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
}

void
sys_set_auto_lock_disabled(int disable)
{
}

void
sys_audio_lock(void)
{
	AcquireSRWLockExclusive(&OS_AUDIO_LOCK);
}

void
sys_audio_unlock(void)
{
	ReleaseSRWLockExclusive(&OS_AUDIO_LOCK);
}

str8
sys_get_current_path(struct alloc alloc)
{
	DWORD needed = GetCurrentDirectoryW(0, NULL);
	if(needed == 0) {
		return (str8){0};
	}
	marena_reset(&OS_STATE.scratch_arena);
	WCHAR *wide  = alloc_arr(OS_STATE.scratch, wide, needed);
	DWORD length = GetCurrentDirectoryW(needed, wide);
	if(length == 0 || length >= needed) {
		marena_reset(&OS_STATE.scratch_arena);
		return (str8){0};
	}
	str8 res = str8_from_16(alloc, (str16){(u16 *)wide, length});
	marena_reset(&OS_STATE.scratch_arena);
	return res;
}

#if !defined(SYS_LOG_DISABLE)
// OutputDebugStringA needs the full text; preformat through the portable fallback.
void
sys_printf(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	sys_log_printf_v(fmt, args);
	va_end(args);
}
#endif

void
sys_log_os_console(const char *text, b32 raw, u32 level)
{
	FILE *stream = raw ? stdout : stderr;
	fputs(text, stream);
	fflush(stream);
	OutputDebugStringA(text);
}

void
sys_log_os_panic(const char *msg)
{
	abort();
}

#if SYS_GFX
#include "sys/sys-gamepad.c"
#include "sys/sys-keyboard.c"
#endif
#if SYS_GFX_SOKOL
#include "sys/sokol/sys-sokol-host.c"
#endif
