#pragma once

#include "base/base-inc.h"
#include "sys/sys-defs.h"

// Host-init hook still called from the optional Sokol helper (before host arenas).
void sys_os_init(void);

// @per_os_impl console sink
// NOTE: Playdate parity: logToConsole always appends a linebreak.
void sys_log_to_console(u32 log_level, str8 message);

// @per_os_impl
void sys_abort(void);

// @per_os_impl
void *sys_alloc(void *ptr, ssize size, ssize align);
void sys_free(void *ptr);
struct alloc sys_allocator(void);

// @per_os_impl Time
f32 sys_time_elapsed(void);
void sys_time_elapsed_reset(void);
u32 sys_time_ms(void);
u32 sys_time_us(void);
u32 sys_epoch_2000(u32 *milliseconds);

// @per_os_impl Paths / process / device integration
str8 sys_get_current_path(struct alloc alloc);
struct sys_process_info *sys_process_info(void);
void sys_set_auto_lock_disabled(int disabled);

// @per_os_impl Audio exclusivity
// Nonrecursive exclusion with app_audio; do not acquire from the mixer.
void sys_audio_lock(void);
void sys_audio_unlock(void);
