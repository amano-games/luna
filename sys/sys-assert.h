#pragma once

#include "sys-log.h"

#if defined(TARGET_PLAYDATE) // assertions don't work on hardware - disable
#undef assert
#undef static_assert
#define assert(X)

// https: //stackoverflow.com/questions/53923706/workaround-for-semicolon-in-global-scope-warning-for-no-op-c-macro
#define static_assert(A, B) struct GlobalScopeNoopTrick
#else
#include <assert.h>
#undef assert
#define assert(c) \
	while(!(c)) __builtin_unreachable()
#endif

#ifdef TARGET_PLAYDATE
#define FILE_AND_LINE ""
#else
#define FILE_AND_LINE__(A, B) A "|" #B
#define FILE_AND_LINE_(A, B)  FILE_AND_LINE__(A, B)
#define FILE_AND_LINE         FILE_AND_LINE_(__FILE__, __LINE__)
#endif

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
