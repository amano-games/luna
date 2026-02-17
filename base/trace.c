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
	spall_auto_thread_init(0, SPALL_DEFAULT_BUFFER_SIZE);
#endif
#if defined(TRACE)
	log_info("trace", "Init (Manual)");
	void *file_handle = sys_file_open_w(file_name);
	dbg_check(file_handle, "trace", "failed to open file: %s", file_name.str);
#endif

	goto error;
error:;
}

void
trace_close(void)
{
	log_info("trace", "closing trace");
#if defined(TRACE_AUTO)
	spall_auto_thread_quit();
	spall_auto_quit();
#endif
#if defined(TRACE)
#endif
}
