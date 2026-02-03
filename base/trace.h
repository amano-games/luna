#pragma once

#include "base/log.h"
#include "base/types.h"

#if defined(TRACE_AUTO)
#define SPALL_AUTO_IMPLEMENTATION
#include "spall_native_auto.h"
#endif

#if defined(TRACE)
#include "sys/sys-io.h"
#include "spall.h"
static SpallProfile SPALL_CTX;
static SpallBuffer SPALL_BUFFER;
#endif

void trace_ini(str8 file_name, u8 *buffer, usize size);
void trace_close(void);

#if defined(TRACE)
#define TRACE_START(s) spall_buffer_begin( \
	&SPALL_CTX, \
	&SPALL_BUFFER, \
	s, \
	sizeof(s) - 1, \
	sys_time_ns())

#define TRACE_END() spall_buffer_end( \
	&SPALL_CTX, \
	&SPALL_BUFFER, \
	sys_time_ns())
#else
#define TRACE_START(...)
#define TRACE_END(...)
#endif

#if defined(TRACE)
SPALL_NOINSTRUMENT
bool
trace_fwrite(SpallProfile *self, const void *p, size_t length)
{
	int res = sys_file_w(self->data, p, length);
	return res;
}

SPALL_NOINSTRUMENT
bool
trace_fflush(SpallProfile *self)
{
	return sys_file_flush(self->data);
}

SPALL_NOINSTRUMENT
void
trace_fclose(SpallProfile *self)
{
	sys_file_flush(self->data);
	sys_file_close(self->data);
}
#endif
