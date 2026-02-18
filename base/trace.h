#pragma once

#include "base/types.h"

#if defined(TRACE_AUTO)
#define SPALL_AUTO_IMPLEMENTATION
#include "spall_native_auto.h"
#endif

void trace_ini(str8 file_name, u8 *buffer, usize size);
void trace_close(void);
