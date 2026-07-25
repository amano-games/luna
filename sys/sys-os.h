#pragma once

#include "base/base-inc.h"
#include "sys/sys-defs.h"

// Host-init hook still called from the optional Sokol helper (before host arenas).
// Public @per_os_impl (alloc/files/paths/epoch/time) live in each OS TU.

void sys_os_init(void);
