#include "sys/sys-log.h"
#include "base/str.h"

#if !defined(SYS_LOG_DISABLE)
void
sys_log_printf_v(const char *fmt, va_list args)
{
	char text[SYS_LOG_TEXT_SIZE];
	usize cap         = sizeof(text) - 2; // reserve for '\n' and NUL
	int size          = sys_vsnprintf(text, (int)cap, fmt, args);
	usize written     = (size >= 0 && (usize)size < cap) ? (usize)size : cap - 1;
	text[written]     = '\n';
	text[written + 1] = 0;
	sys_log_os_console(text, true, SYS_LOG_LEVEL_INFO);
}
#endif

void
sys_log(const char *tag, enum sys_log_level level, u32 item, const char *msg, u32 line, const char *filename)
{

#if !defined(SYS_LOG_DISABLE)
	if(level <= SYS_LOG_LEVEL) {
		const char *severity = "info";
		switch(level) {
		case SYS_LOG_LEVEL_PANI: severity = "panic"; break;
		case SYS_LOG_LEVEL_ERROR: severity = "error"; break;
		case SYS_LOG_LEVEL_WARN: severity = "warning"; break;
		default: break;
		}

		char text[SYS_LOG_TEXT_SIZE];
		if(filename) {
			sys_snprintf(text, sizeof(text) - 1, "[%s][%s][id:%u] %s:%u:0: %s", tag ? tag : "", severity, (uint)item, filename, (uint)line, msg ? msg : "");
		} else {
			sys_snprintf(text, sizeof(text) - 1, "[%s][%s][id:%u][line:%u] %s", tag ? tag : "", severity, (uint)item, (uint)line, msg ? msg : "");
		}

		usize size     = cstr8_len((u8 *)text);
		text[size]     = '\n';
		text[size + 1] = 0;
		sys_log_os_console(text, false, (u32)level);
	}
#endif

	if(level == SYS_LOG_LEVEL_PANI) {
		sys_log_os_panic(msg);
	}
}

void
sys_log_func(const char *tag, u32 level, u32 item, const char *msg, u32 line, const char *filename, void *user_data)
{
	sys_log(tag, (enum sys_log_level)level, item, msg, line, filename);
}
