#pragma once

#if OS_PLAYDATE
extern void (*PD_SYS_LOG_TO_CONSOLE)(const char *fmt, ...);
#define sys_printf(...) PD_SYS_LOG_TO_CONSOLE(__VA_ARGS__)
#else
#include <stdio.h>
// WARN: Playdate always appends a linebreak and there is no way to disable it on C :(
// https://devforum.play.date/t/logtoconsole-without-a-linebreak/1819
#define sys_printf(...) (printf(__VA_ARGS__), printf("\n"))
#endif
