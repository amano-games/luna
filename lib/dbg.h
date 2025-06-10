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
#define dbg_not_implemeneted(T) \
	do { \
		log_error(T, "+++ NOT IMPLEMENTED +++" FILE_AND_LINE); \
		assert(0); \
		goto error; \
	} while(0);

// https://learncodethehardway.com/client/#/lesson/9725/learn-c-the-hard-way-lesson-exercise-19-zeds-awesome-debug-macros/
#define dbg_check(A, ...) \
	if(!(A)) { \
		log_error(__VA_ARGS__); \
		assert(0); \
		goto error; \
	}

#define dbg_sentinel(T) \
	do { \
		log_error(T, "+++ BAD PATH +++"); \
		assert(0); \
		goto error; \
	} while(0);

#define dbg_check_mem(A, T) dbg_check((A), (T), "Out of memory.")
