#include "trace.h"
#include "sys-io.h"

#if !defined(AUTO_TRACE)
SPALL_NOINSTRUMENT
bool
trace_fwrite(SpallProfile *self, const void *p, size_t length)
{
	int res = sys_file_write(self->data, p, length);
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
trace_init(char *file_name, u8 *buffer, usize size)
{

#if defined(AUTO_TRACE)
	log_info("Trace", "Init (Auto)");

	spall_auto_init(file_name);
	int thread_id = 0;
	spall_auto_thread_init(thread_id, SPALL_DEFAULT_BUFFER_SIZE);
#else
#if DEBUG
	log_info("Trace", "Init (Manual)");
	void *f   = sys_file_open(file_name, SYS_FILE_W);
	SPALL_CTX = spall_init_callbacks(1, &trace_fwrite, &trace_fflush, &trace_fclose, f, false);
	// SPALL_CTX    = spall_init_file("pinball-trace.spall", 1);
	SPALL_BUFFER = (SpallBuffer){
		.length = size,
		.data   = buffer,
	};
	spall_buffer_init(&SPALL_CTX, &SPALL_BUFFER);
#endif
#endif
}

void
trace_buffer_close(void)
{
#if defined(AUTO_TRACE)
#else
#if DEBUG
	spall_buffer_quit(&SPALL_CTX, &SPALL_BUFFER);
#endif
#endif
}

void
trace_close(void)
{
#if defined(AUTO_TRACE)
	spall_auto_thread_quit();
	spall_auto_quit();
#else
#if DEBUG
	spall_quit(&SPALL_CTX);
#endif
#endif
}

double
trace_get_time_in_micros(void)
{
#if DEBUG
	return (((double)sys_seconds() * 1000000));
#else
	return 0;
#endif
}
