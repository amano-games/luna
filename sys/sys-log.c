#include "sys-log.h"
#include "sys-backend.h"

void
sys_log(const char *tag, u32 log_level, u32 log_item, const char *str, uint32_t line_nr, const char *filename)
{

	if(log_level <= SYS_LOG_LEVEL) {
		backend_log(tag, log_level, log_item, str, line_nr, filename);
	}
}
