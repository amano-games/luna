#include "sys/sys-log.h"
#include "sys/sys-io.h"

// NOTE: We could create our own sys-log and use it everywhere instead of sokol slog_func
// adding support for playdate using sys_printf (PD_LOG_TO_CONSOLE)
// Sokol gives some features, like android and wasm logging.
// It has a fast append instead of using sys_vsnprintf
// And handles printing correctly to each backend

static sys_file SYS_LOG_FILE_HANDLE = {0};

b32
sys_log_ini(struct alloc alloc, str8 path)
{
	b32 res = false;
	if(SYS_LOG_FILE) {
		if(sys_file_is_valid(SYS_LOG_FILE_HANDLE)) { return true; }
		if(path.size == 0) { return false; }

		str8 dir = str8_chop_last_slash(path);
		if(dir.size > 0) { sys_make_dirs(dir); }

		SYS_LOG_FILE_HANDLE = sys_file_open_w(path);
		res                 = sys_file_is_valid(SYS_LOG_FILE_HANDLE);
		if(res) {
			log_info("sys-log", "log file path: %s", path);
		} else {
			log_warn("sys-log", "failed to open log file path: %s", path);
		}
	}

	return res;
}

// NOTE: call once per backend on shutdown (sokol_cleanup / kEventTerminate / drm main exit)
void
sys_log_close(void)
{
	if(sys_file_is_valid(SYS_LOG_FILE_HANDLE)) {
		sys_file_flush(SYS_LOG_FILE_HANDLE);
		sys_file_close(SYS_LOG_FILE_HANDLE);
		SYS_LOG_FILE_HANDLE = sys_file_zero();
	}
}

// NOTE: poor mans log to file
static void
sys_log_to_file(const char *tag, enum sys_log_level log_level, u32 log_item, const char *msg, uint32_t line_nr, const char *filename, void *userdata)
{

	if(!sys_file_is_valid(SYS_LOG_FILE_HANDLE)) { return; }

	const char *log_level_str;
	switch(log_level) {
	case 0: log_level_str = "panic"; break;
	case 1: log_level_str = "error"; break;
	case 2: log_level_str = "warning"; break;
	default: log_level_str = "info"; break;
	}

	char strret[512]; // tripple buffering sys_lofg/sokol
	sys_snprintf(strret, sizeof(strret) - 1, "[%s] %s:%d\n %s: %s\n", log_level_str, filename, (int)line_nr, tag, msg);
	sys_file_w(SYS_LOG_FILE_HANDLE, strret, cstr8_len((u8 *)strret));
	sys_file_flush(SYS_LOG_FILE_HANDLE);
}

#if !OS_PLAYDATE

#define SOKOL_LOG_IMPL
#include "sokol/sokol_log.h"
void
sys_log(const char *tag, enum sys_log_level log_level, u32 log_item, const char *msg, uint32_t line_nr, const char *filename, void *userdata)
{
	if(log_level > SYS_LOG_LEVEL) { return; }
#if SYS_LOG_STDIO
	slog_func(tag, log_level, log_item, msg, line_nr, filename, userdata);
#endif
#if SYS_LOG_FILE
	sys_log_to_file(tag, log_level, log_item, msg, line_nr, filename, userdata);
#endif
}

#else

void
sys_log(const char *tag, enum sys_log_level log_level, u32 log_item, const char *msg, uint32_t line_nr, const char *filename, void *userdata)
{
	if(log_level > SYS_LOG_LEVEL) { return; }

	const char *log_level_str;
	switch(log_level) {
	case 0: log_level_str = "panic"; break;
	case 1: log_level_str = "error"; break;
	case 2: log_level_str = "warning"; break;
	default: log_level_str = "info"; break;
	}

#if SYS_LOG_STDIO
#if defined(DEBUG_BUILD)
	sys_printf("[%s] %s:%d\n %s: %s", log_level_str, filename, (int)line_nr, tag, msg);
#else
	sys_printf("[%s] %s: %s", log_level_str, tag, msg);
#endif
#endif

#if SYS_LOG_FILE
	sys_log_to_file(tag, log_level, log_item, msg, line_nr, filename, userdata);
#endif
}
#endif
