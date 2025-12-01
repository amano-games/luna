#include "trace.h"
#include "sys/sys-io.h"
#include "base/log.h"
#include "sys/sys.h"

#if !defined(TRACE_AUTO)
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

void
trace_ini(str8 file_name, u8 *buffer, usize size)
{

#if defined(TRACE_AUTO)
	log_info("Trace", "Init (Auto)");

	spall_auto_init(file_name);
	int thread_id = 0;
	spall_auto_thread_init(thread_id, SPALL_DEFAULT_BUFFER_SIZE);
#else
	log_info("trace", "Init (Manual)");
	void *f   = sys_file_open_w(file_name);
	SPALL_CTX = spall_init_callbacks(
		1,
		&trace_fwrite,
		&trace_fflush,
		&trace_fclose,
		f,
		false);
	SPALL_BUFFER = (SpallBuffer){
		.length = size,
		.data   = buffer,
	};
	spall_buffer_init(&SPALL_CTX, &SPALL_BUFFER);
#endif
}

void
trace_buffer_close(void)
{
#if !defined(TRACE_AUTO)
	spall_buffer_quit(&SPALL_CTX, &SPALL_BUFFER);
#endif
}

void
trace_close(void)
{
#if defined(TRACE_AUTO)
	spall_auto_thread_quit();
	spall_auto_quit();
#else
	spall_quit(&SPALL_CTX);
#endif
}

double
trace_get_time_in_micros(void)
{
	return (((double)sys_seconds() * 1000000));
}
