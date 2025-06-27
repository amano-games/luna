#pragma once

#if SYS_LOG_DISABLE
#define sys_printf(...)
#else
#define sys_printf printf
#endif
