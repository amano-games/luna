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

#define log_info(tag, ...)  __sys_log(tag, SYS_LOG_LEVEL_INFO, __VA_ARGS__);
#define log_warn(tag, ...)  __sys_log(tag, SYS_LOG_LEVEL_WARN, __VA_ARGS__);
#define log_error(tag, ...) __sys_log(tag, SYS_LOG_LEVEL_ERROR, __VA_ARGS__);

#if DISABLE_LOGGING
#define __sys_log(tag, level, ...) {};
#else
#define __sys_log(tag, level, ...) \
	{ \
		char strret[1024] = {0}; \
		sys_snprintf(strret, sizeof(strret) - 1, __VA_ARGS__); \
		sys_log(tag, level, 0, strret, __LINE__, __FILE__); \
	}
#endif
