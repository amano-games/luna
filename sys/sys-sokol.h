#pragma once

#if SYS_LOG_DISABLE
#define sys_printf(...)
#else
#include <stdio.h>
#define sys_printf printf
#endif
