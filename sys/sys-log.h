#pragma once

#include "sys-types.h"

enum sys_log_level {
	SYS_LOG_LEVEL_PANI  = 0,
	SYS_LOG_LEVEL_ERROR = 1,
	SYS_LOG_LEVEL_WARN  = 2,
	SYS_LOG_LEVEL_INFO  = 3,
};

#if !defined(SYS_LOG_LEVEL)
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_WARN
#endif

void sys_log(const char *tag, enum sys_log_level log_level, u32 log_item, const char *msg, uint32_t line_nr, const char *filename);
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
	if(log_level > SYS_LOG_LEVEL)
		return;

	char strret[1024];
	va_list args;
	va_start(args, fmt);
	sys_vsnprintf(strret, sizeof(strret) - 1, fmt, args);
	va_end(args);

	sys_log(tag, log_level, log_item, strret, line_nr, filename);
}

#if DISABLE_LOGGING
#define __sys_log(tag, level, ...) {};
#else
#define __sys_log(tag, level, ...) \
	{ \
		sys_logf(tag, level, 0, __LINE__, __FILE__, __VA_ARGS__); \
	}
#endif

#define log_info(tag, ...)  __sys_log(tag, SYS_LOG_LEVEL_INFO, __VA_ARGS__);
#define log_warn(tag, ...)  __sys_log(tag, SYS_LOG_LEVEL_WARN, __VA_ARGS__);
#define log_error(tag, ...) __sys_log(tag, SYS_LOG_LEVEL_ERROR, __VA_ARGS__);
