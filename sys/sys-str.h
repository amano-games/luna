#pragma once

#include "base/base-inc.h"

#if OS_PLAYDATE
extern int (*PD_SYS_PARSE_STR)(const char *str, const char *format, ...);
#define sys_parse_string(str, fmt, ...) PD_SYS_PARSE_STR(str, fmt, __VA_ARGS__);
#else
#include <stdio.h>
#define sys_parse_string(str, fmt, ...) sscanf(str, fmt, __VA_ARGS__);
#endif
