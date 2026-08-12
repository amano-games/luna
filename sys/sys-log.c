#define SOKOL_LOG_IMPL
#include "sokol/sokol_log.h"
#undef SOKOL_LOG_IMPL

#include "sys/sys-log.h"

void
sys_log(
	const char *tag,
	enum sys_log_level log_level,
	u32 log_item,
	const char *msg,
	u32 line_nr,
	const char *filename)
{
	if(log_level <= SYS_LOG_LEVEL) {
		slog_func(tag, log_level, log_item, msg, line_nr, filename, NULL);
	}
}
