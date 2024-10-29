#pragma once

#include "sys-log.h"

#if defined(TARGET_PLAYDATE) // assertions don't work on hardware - disable
#undef assert
#define assert(X)
#else
#undef assert
#define assert(c) \
	while(!(c)) __builtin_unreachable()
#endif

#define FILE_AND_LINE__(A, B) A "|" #B
#define FILE_AND_LINE_(A, B)  FILE_AND_LINE__(A, B)
#define FILE_AND_LINE         FILE_AND_LINE_(__FILE__, __LINE__)

// TODO: move to log_panic
#define NOT_IMPLEMENTED \
	do { \
		sys_printf("+++ NOT IMPLEMENTED +++"); \
		sys_printf(FILE_AND_LINE); \
		assert(0); \
	} while(0);

#define BAD_PATH \
	do { \
		sys_printf("+++ BAD PATH +++"); \
		sys_printf(FILE_AND_LINE); \
		assert(0); \
	} while(0);
