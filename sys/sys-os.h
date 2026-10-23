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
// https://randomascii.wordpress.com/2012/02/13/dont-store-that-in-a-float/
// https://tomforsyth1000.github.io/blog.wiki.html#%5B%5BA%20matter%20of%20precision%5D%5D
u64 sys_time_us(void);
void sys_sleep_ms(u32 ms);
u32 sys_epoch_2000(u32 *milliseconds);

static inline f32
sys_time_sec_since(u64 start_us)
{
	return (f32)(sys_time_us() - start_us) * 1e-6f;
}

// @per_os_impl Paths / process / device integration
str8 sys_get_current_path(struct alloc alloc);
struct sys_process_info *sys_process_info(void);
void sys_set_auto_lock_disabled(int disabled);

// @per_os_impl Dynamic libraries
typedef void sys_proc(void);
typedef struct sys_lib sys_lib;
struct sys_lib {
	u64 u64[1];
};

sys_lib sys_lib_open(str8 path);
void sys_lib_close(sys_lib lib);
sys_proc *sys_lib_load_proc(sys_lib lib, str8 name);

static inline b32
sys_lib_is_valid(sys_lib lib)
{
	return lib.u64[0] != 0;
}

// @per_os_impl Audio exclusivity
// Nonrecursive exclusion with app_audio; do not acquire from the mixer.
void sys_audio_lock(void);
void sys_audio_unlock(void);
