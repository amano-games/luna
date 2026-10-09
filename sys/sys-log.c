#include "sys/sys-log.h"
#include "base/marena.h"
#include "sys/sys-io.h"

static sys_file SYS_LOG_FILE_HANDLE = {0};

void
sys_log_to_file(const char *tag, enum sys_log_level log_level, u32 log_item, const char *msg, uint32_t line_nr, const char *filename, void *userdata)
{
// NOTE: Poor mans log to file, where does this log go?
// Should we have a log_ini/close?
// Should we have a luna_set_app_name/org_name so that the log files are created at logs/app_name/log.txt?
// Should we enable log to file always or be a ENUM?
// Should we have a way to enable disable logs at runtime? game has one log file luna has another one?
// sys_file_w seems to be really slow
// Luna logs and app logs are the same if the game crashes because of a luna log I want to have that data on the same file ideally on the game log folder.
// Maybe by default is sys_logs_folder/luna/log.txt but the game can set
// sys_log_file_set_path(str8 path);
// But how early can we do this?
// What if luna crashes before the game starts
// Ideally I would like to have the logs on dev console/file/screen and turn that on/off
#if 0
	b32 res = false;

	if(!sys_file_is_valid(SYS_LOG_FILE_HANDLE)) {
		marena_stack(arena, 1024);
		struct alloc alloc  = marena_allocator(&arena);
		str8 path           = str8_lit("luna-log.txt");
		path                = sys_path_to_log_path(alloc, path, str8_lit(""), str8_lit(""));
		SYS_LOG_FILE_HANDLE = sys_file_open_w(path);
		res                 = sys_file_is_valid(SYS_LOG_FILE_HANDLE);
	}

	if(sys_file_is_valid(SYS_LOG_FILE_HANDLE)) {
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
#endif
}

// NOTE: We could create our own sys-log and use it everywhere instead of sokol slog_func
// adding support for playdate using sys_printf (PD_LOG_TO_CONSOLE)
// Sokol gives some features, like android and wasm logging.
// It has a fast append instead of using sys_vsnprintf
// And handles printing correctly to each backend
#if !OS_PLAYDATE

#define SOKOL_LOG_IMPL
#include "sokol/sokol_log.h"
void
sys_log(const char *tag, enum sys_log_level log_level, u32 log_item, const char *msg, uint32_t line_nr, const char *filename, void *userdata)
{
	if(log_level > SYS_LOG_LEVEL) { return; }
	slog_func(tag, log_level, log_item, msg, line_nr, filename, userdata);
	sys_log_to_file(tag, log_level, log_item, msg, line_nr, filename, userdata);
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

#if defined(DEBUG_BUILD)
	sys_printf("[%s] %s:%d\n %s: %s", log_level_str, filename, (int)line_nr, tag, msg);
#else
	sys_printf("[%s] %s: %s", log_level_str, tag, msg);
#endif
}
#endif
