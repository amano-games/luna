#include "sys/sys-log.h"

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
	slog_func(tag, log_level, log_item, msg, line_nr, filename, userdata);
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
