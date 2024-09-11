#pragma once

#include "sys-log.h"
#include "sys-types.h"

#define FILE_PATH_GEN(NAME, PATH_NAME, FILE_NAME) \
	char NAME[64]; \
	str_cpy(NAME, PATH_NAME); \
	str_append(NAME, FILE_NAME)

static void
str_cpy(char *dst, const char *src)
{
	char *d       = dst;
	const char *s = src;

	while(*s != '\0')
		*d++ = *s++;

	*d = '\0';
}

static i32
str_len(const char *a)
{
	for(i32 l = 0;; l++)
		if(a[l] == '\0') return l;
	return 0;
}

// appends string b -> overwrites null-char and places a new null-char
static void
str_append(char *dst, const char *src)
{
	str_cpy(&dst[str_len(dst)], src);
}

static i32
str_eq(const char *a, const char *b)
{
	const char *x = a;
	const char *y = b;
	while(*x != '\0') {
		if(*x != *y) return 0;
		x++, y++;
	}
	return 1;
}

// https://github.com/gcc-mirror/gcc/blob/master/libiberty/strncmp.c
static i32
str_cmp(const char *s1, const char *s2, register usize n)
{
	register u8 u1, u2;

	while(n-- > 0) {
		u1 = (u8)*s1++;
		u2 = (u8)*s2++;
		if(u1 != u2)
			return u1 - u2;
		if(u1 == '\0')
			return 0;
	}
	return 0;
}

#define S_(x)   #x
#define S(x)    S_(x)
#define PBWIDTH 20
#define PBCHAR  '#'

static void
ascii_progress_bar(u8 percent)
{
	if(SYS_LOG_LEVEL > 2) {
		char pbstr[PBWIDTH];
		memset(pbstr, PBCHAR, PBWIDTH);
		sys_printf("[%-" S(PBWIDTH) ".*s] %u%%", percent * PBWIDTH / 100, pbstr, percent);
		sys_printf(" ");
	}
}

f32
str_to_f32(string str)
{
	f32 num   = 0.0;
	f32 mul   = 1.0;
	i32 dec   = 0;
	usize len = str.len;

	for(usize i = 0; i < len; i++) {
		if(str.data[i] == '.') dec = 1;
	}

	for(int idx = len - 1; idx >= 0; idx--) {
		if(str.data[idx] == '-')
			num = -num;
		else if(str.data[idx] == '.')
			dec = 0;
		else if(dec) {
			num += str.data[idx] - '0';
			num *= 0.1f;
		} else {
			num += (str.data[idx] - '0') * mul;
			mul *= 10.0f;
		}
	}

	return num;
}

static inline string
str_from(char *data)
{
	return (string){
		.len  = str_len(data),
		.data = data,
	};
}
