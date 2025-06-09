#pragma once

#include "sys-types.h"
#if defined(AUTO_TRACE)
#define SPALL_AUTO_IMPLEMENTATION
#include "spall_native_auto.h"
#else
#include "spall.h"
static SpallProfile SPALL_CTX;
static SpallBuffer SPALL_BUFFER;
#endif

void trace_init(str8 file_name, uchar *buffer, usize size);
void trace_buffer_close(void);
void trace_close(void);

double trace_get_time_in_micros(void);

#if DEBUG && !defined(AUTO_TRACE)
#define TRACE_START(s) spall_buffer_begin( \
	&SPALL_CTX, \
	&SPALL_BUFFER, \
	s, \
	sizeof(s) - 1, \
	trace_get_time_in_micros())

#define TRACE_END() spall_buffer_end( \
	&SPALL_CTX, \
	&SPALL_BUFFER, \
	trace_get_time_in_micros())
#else
#define TRACE_START(...)
#define TRACE_END(...)
#endif
