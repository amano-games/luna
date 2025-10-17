#pragma once

#include "base/types.h"

enum sys_log_level {
	SYS_LOG_LEVEL_PANI  = 0,
	SYS_LOG_LEVEL_ERROR = 1,
	SYS_LOG_LEVEL_WARN  = 2,
	SYS_LOG_LEVEL_INFO  = 3,
};

#if !defined(SYS_LOG_LEVEL)
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_WARN
#endif

#if SYS_LOG_DISABLE
#define sys_printf(...)
#else
#if TARGET_PLAYDATE
#include "pd-sys.h"
#define sys_printf(...) PD_SYSTEM_LOG_TO_CONSOLE(__VA_ARGS__)
#else
#include <stdio.h>
#define sys_printf(...) (printf(__VA_ARGS__), printf("\n"))
#endif
#endif

void sys_log(const char *tag, enum sys_log_level log_level, u32 log_item, const char *msg, uint32_t line_nr, const char *filename);

// TODO: Add __attribute__(format(gnu_printf, 6, 7)))
// for static validation
// https://github.com/nothings/stb/issues/1814
static inline void
sys_logf(
	const char *tag,
	enum sys_log_level log_level,
	u32 log_item,
	uint32_t line_nr,
	const char *filename,
	const char *fmt,
	...)
{
#if !defined(SYS_LOG_DISABLE)
	if(log_level > SYS_LOG_LEVEL) { return; }

	char strret[1024];
	va_list args;
	va_start(args, fmt);
	sys_vsnprintf(strret, sizeof(strret) - 1, fmt, args);
	va_end(args);

	sys_log(tag, log_level, log_item, strret, line_nr, filename);
#endif
}

#define log_info(tag, ...)  sys_logf(tag, SYS_LOG_LEVEL_INFO, 0, __LINE__, __FILE__, __VA_ARGS__);
#define log_warn(tag, ...)  sys_logf(tag, SYS_LOG_LEVEL_WARN, 0, __LINE__, __FILE__, __VA_ARGS__);
#define log_error(tag, ...) sys_logf(tag, SYS_LOG_LEVEL_ERROR, 0, __LINE__, __FILE__, __VA_ARGS__);
