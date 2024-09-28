#pragma once

#if defined(BACKEND_SOKOL)
#include <stdio.h>
#define sys_parse_string(str, fmt, ...) sscanf(str, fmt, __VA_ARGS__);
#else
#define sys_parse_string(str, fmt, ...) PD_SYSTEM_PARSE_STR(str, fmt, __VA_ARGS__);
#endif
