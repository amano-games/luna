#define STB_SPRINTF_IMPLEMENTATION
#define STB_SPRINTF_NOUNALIGNED
#include "stb_sprintf.h"

#include "sys/sys-sprintf.h"

int
sys_vsprintf(char *buf, char const *fmt, va_list va)
{
	return stbsp_vsprintf(buf, fmt, va);
}

int
sys_vsnprintf(char *buf, int count, char const *fmt, va_list va)
{
	return stbsp_vsnprintf(buf, count, fmt, va);
}

int
sys_sprintf(char *buf, char const *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	int result = stbsp_vsprintf(buf, fmt, va);
	va_end(va);
	return result;
}

int
sys_snprintf(char *buf, int count, char const *fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	int result = stbsp_vsnprintf(buf, count, fmt, va);
	va_end(va);
	return result;
}
