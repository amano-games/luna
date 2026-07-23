#pragma once

#include <stdarg.h>

#if defined(__has_attribute)
#if __has_attribute(format)
#define SYS_SPRINTF_FORMAT(fmt, va) __attribute__((format(printf, fmt, va)))
#endif
#endif

#ifndef SYS_SPRINTF_FORMAT
#define SYS_SPRINTF_FORMAT(fmt, va)
#endif

int sys_vsprintf(char *buf, char const *fmt, va_list va);
int sys_vsnprintf(char *buf, int count, char const *fmt, va_list va);
int sys_sprintf(char *buf, char const *fmt, ...) SYS_SPRINTF_FORMAT(2, 3);
int sys_snprintf(char *buf, int count, char const *fmt, ...) SYS_SPRINTF_FORMAT(3, 4);
