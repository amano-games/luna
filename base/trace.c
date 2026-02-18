#include "trace.h"

#if defined(TRACE_AUTO)
#include "base/log.h"
#include "spall_native_auto.h"
#endif

void
trace_ini(str8 file_name, u8 *buffer, usize size)
{
#if defined(TRACE_AUTO)
	log_info("Trace", "Init (Auto)");
	spall_auto_init((char *)file_name.str);
	spall_auto_thread_init(0, SPALL_DEFAULT_BUFFER_SIZE);
#endif
}

void
trace_close(void)
{
#if defined(TRACE_AUTO)
	log_info("trace", "closing trace");
	spall_auto_thread_quit();
	spall_auto_quit();
#endif
}
