#include "sys/sys-log.h"

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

	const char *log_level_str = NULL;
	switch(log_level) {
	case SYS_LOG_LEVEL_PANI: log_level_str = "PANI"; break;
	case SYS_LOG_LEVEL_ERROR: log_level_str = "ERRO"; break;
	case SYS_LOG_LEVEL_WARN: log_level_str = "WARN"; break;
	default: log_level_str = "INFO"; break;
	}

#if defined(DEV)
	sys_printf("[%s] %s:%d\n %s: %s", log_level_str, filename, (int)line_nr, tag, msg);
#else
	sys_printf("[%s] %s: %s", log_level_str, tag, msg);
#endif
}
#endif
