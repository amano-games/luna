// @per_os_impl WASM — owns OS sys_* ; optional Sokol helper for present/window/audio.

#include "base/types.h"
#include "sys/sys-log.h"
#include "base/marena.h"
#include "base/mem.h"
#include "base/path.h"
#include "base/str.h"
#include "sys/sys-defs.h"
#include "sys/sys-io.h"
#include "sys/sys-mem.h"
#include "sys/sys-os.h"
#include "sys/sys.h"

#if SYS_GFX
#include "sys/sys-gamepad.h"
#include "sys/sys-keyboard.h"
#endif

#include <emscripten/emscripten.h> // EM_JS
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
} OS_STATE;

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
sys_audio_lock(void)
{
}

void
sys_audio_unlock(void)
{
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
	struct sys_process_info *info = &OS_STATE.process_info;
	*info                         = (struct sys_process_info){0};
	info->pid                     = (u32)getpid();
	info->initial_path            = sys_get_current_path(alloc_sys);

	stm_setup();
	OS_STATE.tick_start = stm_now();

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
	char path_copy[1024];
	dbg_assert(path.size < sizeof(path_copy));
	if(!path.str || path.size == 0 || path.size >= sizeof(path_copy)) { return false; }
	memcpy(path_copy, path.str, (usize)path.size);
	path_copy[path.size] = 0;

	b32 result = 0;
	if(mkdir(path_copy, 0755) != -1) {
		result = 1;
	} else {
		struct stat info = {0};
		result           = stat(path_copy, &info) == 0 && S_ISDIR(info.st_mode);
	}
	return result;
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

u64
sys_time_us(void)
{
	return (u64)stm_us(stm_since(OS_STATE.tick_start));
}

void
sys_sleep_ms(u32 ms)
{
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

static dense_time
sys_dense_time_from_timespec(struct timespec in)
{
	struct tm tm_time = {0};
	gmtime_r(&in.tv_sec, &tm_time);
	struct date_time dt = {
		.year = (u32)tm_time.tm_year + 1900,
		.mon  = (u32)tm_time.tm_mon,
		.day  = (u16)(tm_time.tm_mday - 1),
		.hour = (u16)tm_time.tm_hour,
		.min  = (u16)tm_time.tm_min,
		.sec  = (u16)tm_time.tm_sec,
		.msec = (u16)(in.tv_nsec / 1000000),
	};
	return dense_time_from_date_time(dt);
}

static struct sys_file_props
sys_file_props_from_stat(struct stat *s)
{
	struct sys_file_props res = {0};
	res.size                  = (u64)s->st_size;
	res.modified              = sys_dense_time_from_timespec(s->st_mtim);
	res.created               = sys_dense_time_from_timespec(s->st_ctim); // status change, not birth
	if(S_ISDIR(s->st_mode)) { res.flags |= SYS_FILE_PROP_FLAG_IS_DIR; }
	return res;
}

struct sys_file_props
sys_file_props_from_file(sys_file f)
{
	struct sys_file_props res = {0};
	struct stat st            = {0};
	if(!sys_file_is_valid(f)) { return res; }
	if(fstat(fileno(sys_file_fp(f)), &st) != 0) {
		log_error("IO", "failed to get file stats from handle");
		return res;
	}
	res = sys_file_props_from_stat(&st);
	return res;
}

struct sys_file_props
sys_file_props_from_path(str8 path)
{
	struct sys_file_props res = {0};
	struct stat st            = {0};
	if(stat((char *)path.str, &st) != 0) {
		log_error("IO", "failed to get file stats %.*s", str8_spread(path));
		return res;
	}
	res = sys_file_props_from_stat(&st);
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
	return sys_file_rename(from, to);
}

b32
sys_file_copy(str8 from, str8 to)
{
	return sys_file_copy_chunked(from, to);
}

void
sys_set_auto_lock_disabled(int disable)
{
}

sys_lib
sys_lib_open(str8 path)
{
	log_error("sys-lib", "dynamic libraries not supported: %.*s", str8_spread(path));
	return (sys_lib){0};
}

void
sys_lib_close(sys_lib lib)
{
}

sys_proc *
sys_lib_load_proc(sys_lib lib, str8 name)
{
	return NULL;
}

// slog_js_log
EM_JS(void, sys_log_js_console, (u32 level, const char *c_str), {
	const str = UTF8ToString(c_str);
	switch(level) {
	case 0: console.error(str); break;
	case 1: console.error(str); break;
	case 2: console.warn(str); break;
	default: console.info(str); break;
	}
})

void
sys_log_to_console(u32 log_level, str8 message)
{
	sys_log_js_console(log_level, (const char *)message.str);
	if(log_level == SYS_LOG_LEVEL_PANI) { abort(); }
}

void
sys_abort(void)
{
	abort();
}

#if SYS_GFX
#include "sys/sys-gamepad-stub.c"
#include "sys/sys-keyboard.c"
#endif
#if SYS_GFX_SOKOL
#include "sys/sokol/sys-sokol-host.c"
#endif
