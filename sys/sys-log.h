#pragma once

#include "base/log.h"
#include <stdarg.h>

// Sokol-style console line budget, truncate silently
#define SYS_LOG_TEXT_SIZE (512)

void sys_log_func(const char *tag, u32 level, u32 item, const char *msg, u32 line, const char *filename, void *user_data);

// NOTE: Internal va_list bridge, not for general use — call sys_printf instead.
// Exists because the console APIs of the preformatting OS targets cannot absorb
// variadic output (win: OutputDebugStringA takes a full string, playdate:
// logToConsole has no v-variant, wasm: the leveled JS bridge takes a char*) and
// C cannot forward "..." across a function call.
// linux/macos bypass it entirely and stream with vfprintf instead.
void sys_log_printf_v(const char *fmt, va_list args);

// @per_os_impl Logging sinks. None of these may call back into the logger.
void sys_log_os_console(const char *text, b32 raw, u32 level);
void sys_log_os_panic(const char *msg);
