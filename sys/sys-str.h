#pragma once

#include "sys-backend.h"

#if defined(BACKEND_SOKOL)
#include <stdio.h>
#define sys_parse_string(str, fmt, ...) sscanf(str, fmt, __VA_ARGS__);
#else
extern int (*PD_PARSE_STR)(const char *str, const char *format, ...);
#define sys_parse_string(str, fmt, ...) PD_PARSE_STR(str, fmt, __VA_ARGS__);
#endif
