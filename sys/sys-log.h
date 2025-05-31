#pragma once

#include "sys-types.h"

enum sys_log_level {
	SYS_LOG_LEVEL_PANI  = 0,
	SYS_LOG_LEVEL_ERROR = 1,
	SYS_LOG_LEVEL_WARN  = 2,
	SYS_LOG_LEVEL_INFO  = 3,
};

#define DISABLE_LOGGING 0
#if !defined(SYS_LOG_LEVEL)
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_WARN
#endif

void sys_log(const char *tag, enum sys_log_level log_level, u32 log_item, const char *msg, uint32_t line_nr, const char *filename);

#define log_info(tag, ...)  __sys_log(tag, SYS_LOG_LEVEL_INFO, __VA_ARGS__);
#define log_warn(tag, ...)  __sys_log(tag, SYS_LOG_LEVEL_WARN, __VA_ARGS__);
#define log_error(tag, ...) __sys_log(tag, SYS_LOG_LEVEL_ERROR, __VA_ARGS__);

#if DISABLE_LOGGING
#define sys_printf(...)            {};
#define __sys_log(tag, level, ...) {};
#else
#define __sys_log(tag, level, ...) \
	{ \
		char strret[1024] = {0}; \
		stbsp_snprintf(strret, sizeof(strret) - 1, __VA_ARGS__); \
		sys_log(tag, level, 0, strret, __LINE__, __FILE__); \
	}
#if defined(BACKEND_SOKOL)
#include <stdio.h>
#define sys_printf(...) \
	{ \
		char strret[1024] = {0}; \
		stbsp_snprintf(strret, sizeof(strret) - 1, __VA_ARGS__); \
		printf("%s\n", strret); \
	}
#elif defined(BACKEND_PD)
extern void (*PD_SYSTEM_LOG_TO_CONSOLE)(const char *format, ...);
#define sys_printf(...) \
	{ \
		PD_SYSTEM_LOG_TO_CONSOLE(__VA_ARGS__); \
	}
#else
#include <stdio.h>
#define sys_printf(...) \
	{ \
		char strret[1024] = {0}; \
		stbsp_snprintf(strret, sizeof(strret) - 1, __VA_ARGS__); \
		printf("%s\n", strret); \
	}
#endif
#endif
