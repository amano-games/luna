#pragma once

#include "base/base-inc.h"
#include "sys/sys-defs.h"

// TODO: add more os things to os

// Host-init hook still called from the optional Sokol helper (before host arenas).
// Public @per_os_impl (alloc/files/paths/epoch/time) live in each OS TU.
void sys_os_init(void);

// @per_os_impl console sink
// NOTE: Playdate parity: logToConsole always appends a linebreak.
void sys_log_to_console(u32 log_level, str8 message);

// @per_os_impl dead-man's-switch panic exit
void sys_abort(void);
