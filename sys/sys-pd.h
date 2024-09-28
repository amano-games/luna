#pragma once

#include "sys-types.h"

#include "pd_api.h"
extern PlaydateAPI *PD;

bool32 sys_pd_reduce_flicker(void);
f32 sys_pd_crank_deg(void);
void sys_pd_update_rows(i32 from_incl, i32 to_incl);
// f32 sys_pd_crank(void);
// bool32 sys_pd_crank_docked(void);
// u32 sys_pd_btn(void);
