#pragma once

#include "base/types.h"

#include "pd_api.h"

extern PlaydateAPI *PD;

extern void (*PD_SYS_LOG_TO_CONSOLE)(const char *fmt, ...);

b32 sys_pd_reduce_flicker(void);
f32 sys_pd_crank_deg(void);
void sys_pd_update_rows(i32 from_incl, i32 to_incl);
