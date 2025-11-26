#pragma once

#include "base/log.h"

#if defined(TARGET_PD_DEVICE) // assertions don't work on hardware - disable
#define dbg_assert(X)
#else
#if __GNUC__
#define dbg_assert(c) \
	if(!(c)) __builtin_trap()
#elif _MSC_VER
#define dbg_assert(c) \
	if(!(c)) __debugbreak()
#else
#define dbg_assert(c) \
	if(!(c)) *(volatile int *)0 = 0
#endif
#endif

#define FILE_AND_LINE__(A, B) A "|" #B
#define FILE_AND_LINE_(A, B)  FILE_AND_LINE__(A, B)
#define FILE_AND_LINE         FILE_AND_LINE_(__FILE__, __LINE__)

// TODO: move to log_panic
#define dbg_not_implemeneted(T) \
	do { \
		log_error(T, "+++ NOT IMPLEMENTED +++" FILE_AND_LINE); \
		dbg_assert(0); \
		goto error; \
	} while(0);

// https://learncodethehardway.com/client/#/lesson/9725/learn-c-the-hard-way-lesson-exercise-19-zeds-awesome-debug-macros/
#define dbg_check(A, ...) \
	if(!(A)) { \
		log_error(__VA_ARGS__); \
		dbg_assert(0); \
		goto error; \
	}

#define dbg_check_warn(A, ...) \
	if(!(A)) { \
		log_warn(__VA_ARGS__); \
		goto error; \
	}

#define dbg_sentinel(T) \
	do { \
		log_error(T, "+++ BAD PATH +++"); \
		dbg_assert(0); \
		goto error; \
	} while(0);

#define dbg_check_mem(A, T) dbg_check((A), (T), "Out of memory.")
