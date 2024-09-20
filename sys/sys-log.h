#pragma once

#include "sys-types.h"
#include "str.h"
#include <inttypes.h>

#define DISABLE_LOGGING 0
#if !defined(SYS_LOG_LEVEL)
#define SYS_LOG_LEVEL 3
#endif

void sys_log(const char *tag, u32 log_level, u32 log_item, const char *msg, uint32_t line_nr, const char *filename);

#define log_info(tag, ...)  __sys_log(tag, 3, __VA_ARGS__);
#define log_warn(tag, ...)  __sys_log(tag, 2, __VA_ARGS__);
#define log_error(tag, ...) __sys_log(tag, 1, __VA_ARGS__);

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
#define sys_printf(...) \
	{ \
		char strret[1024] = {0}; \
		stbsp_snprintf(strret, sizeof(strret) - 1, __VA_ARGS__); \
		printf("%s\n", strret); \
	}
#else
extern void (*PD_LOG)(const char *format, ...);
#define sys_printf(...) \
	{ \
		PD_LOG(__VA_ARGS__); \
	}
#endif
#endif
