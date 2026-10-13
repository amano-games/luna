#pragma once

#include <stdarg.h>

#if defined(__has_attribute)
#if __has_attribute(format)
#define SYS_PRINTF_FORMAT(fmt, va) __attribute__((format(printf, fmt, va)))
#endif
#endif

#ifndef SYS_PRINTF_FORMAT
#define SYS_PRINTF_FORMAT(fmt, va)
#endif

int sys_vsprintf(char *buf, char const *fmt, va_list va);
int sys_vsnprintf(char *buf, int count, char const *fmt, va_list va);
int sys_sprintf(char *buf, char const *fmt, ...) SYS_PRINTF_FORMAT(2, 3);
int sys_snprintf(char *buf, int count, char const *fmt, ...) SYS_PRINTF_FORMAT(3, 4);

#if OS_PLAYDATE
extern void (*PD_SYS_LOG_TO_CONSOLE)(const char *fmt, ...);
#define sys_printf(...) PD_SYS_LOG_TO_CONSOLE(__VA_ARGS__)
#else
#include <stdio.h>
// WARN: Playdate always appends a linebreak and there is no way to disable it on C :(
// https://devforum.play.date/t/logtoconsole-without-a-linebreak/1819
#define sys_printf(...) (printf(__VA_ARGS__), printf("\n"))
#endif
