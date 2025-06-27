#pragma once

#if DISABLE_LOGGING
#define sys_printf(...)
#else
#define sys_printf printf
#endif
#include "whereami.h"
