#pragma once

// https://www.frogtoss.com/labs/low-overhead-structured-logging-in-c.html
// https://github.com/tsoding/nob.h/blob/main/nob.h#L2037  nob__log_handler?
// https://github.com/rxi/log.c

#include "base/mem.h"
#include "base/types.h"

#define SYS_LOG_LEVEL_PANI  0
#define SYS_LOG_LEVEL_ERROR 1
#define SYS_LOG_LEVEL_WARN  2
#define SYS_LOG_LEVEL_INFO  3

#if !defined(SYS_LOG_LEVEL)
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_WARN
#endif

#if !defined(SYS_LOG_STDIO)
#define SYS_LOG_STDIO 1
#endif

#if !defined(SYS_LOG_FILE)
#define SYS_LOG_FILE (SYS_LOG_LEVEL <= SYS_LOG_LEVEL_WARN)
#endif

b32 sys_log_ini(struct alloc alloc, str8 path);
void sys_log_close(void);

void sys_log(const char *tag, u32 log_level, u32 log_item, const char *msg, uint32_t line_nr, const char *filename, void *userdata);

// TODO: Add __attribute__(format(gnu_printf, 6, 7)))
// for static validation
// https://github.com/nothings/stb/issues/1814
// https://vimeo.com/855891054
// TODO: we are doing double buffering sys_log is calling sokol_log_func that does the same thing
// But this is probably unavoidable becuase playdate doesnt have a vargs alt
static inline void
sys_logf(
	const char *tag,
	u32 log_level,
	u32 log_item,
	uint32_t line_nr,
	const char *filename,
	const char *fmt,
	...)
{
#if !defined(SYS_LOG_DISABLE)
	if(log_level > SYS_LOG_LEVEL) { return; }

	char strret[512];
	va_list args;
	va_start(args, fmt);
	sys_vsnprintf(strret, sizeof(strret) - 1, fmt, args);
	va_end(args);

	sys_log(tag, log_level, log_item, strret, line_nr, filename, NULL);
#endif
}

#define log_info(tag, ...)  sys_logf(tag, SYS_LOG_LEVEL_INFO, 0, 0, 0, __VA_ARGS__);
#define log_warn(tag, ...)  sys_logf(tag, SYS_LOG_LEVEL_WARN, 0, __LINE__, __FILE__, __VA_ARGS__);
#define log_error(tag, ...) sys_logf(tag, SYS_LOG_LEVEL_ERROR, 0, __LINE__, __FILE__, __VA_ARGS__);
