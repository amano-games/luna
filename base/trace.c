#include "trace.h"
#include "base/log.h"
#if defined(TRACE_AUTO)
#include "spall_native_auto.h"
#else
#include "base/dbg.h"
#include "sys/sys-io.h"
#endif

void
trace_ini(str8 file_name, u8 *buffer, usize size)
{

#if defined(TRACE_AUTO)
	log_info("Trace", "Init (Auto)");

	spall_auto_init((char *)file_name.str);
	int thread_id = 0;
	spall_auto_thread_init(thread_id, SPALL_DEFAULT_BUFFER_SIZE);

#else
	log_info("trace", "Init (Manual)");
	void *f = sys_file_open_w(file_name);
	b32 res = spall_init_callbacks(
		1,
		&trace_fwrite,
		&trace_fflush,
		&trace_fclose,
		f,
		&SPALL_CTX);
	dbg_assert(res);

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
