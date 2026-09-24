#pragma once

#include "engine/cam/cam.h"

#if BUILD_DEBUG && !PD_DEVICE
void dbg_drw_cam(struct cam *c, u8 col);
#else
#define dbg_drw_cam(...) ((void)0)
#endif
