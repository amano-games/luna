#pragma once

#include "base/types.h"

#include "pd_api.h"
extern PlaydateAPI *PD;

extern void (*PD_SYSTEM_LOG_TO_CONSOLE)(const char *fmt, ...);
#if SYS_LOG_DISABLE
#define sys_printf(...)
#else
#define sys_printf PD_SYSTEM_LOG_TO_CONSOLE
#endif

b32 sys_pd_reduce_flicker(void);
f32 sys_pd_crank_deg(void);
void sys_pd_update_rows(i32 from_incl, i32 to_incl);
// f32 sys_pd_crank(void);
// bool32 sys_pd_crank_docked(void);
// u32 sys_pd_btn(void);

// NOLINTBEGIN(readability-identifier-naming)
// make ARM linker shut up about things we aren't using (nosys lib issues):
void
_close(void)
{
}

void
_lseek(void)
{
}

void
_read(void)
{
}

void
_write(void)
{
}

void
_fstat(void)
{
}

void
_getpid(void)
{
}

void
_isatty(void)
{
}

void
_kill(void)
{
}
// NOLINTEND(readability-identifier-naming)
// end ARM linker warning hack
