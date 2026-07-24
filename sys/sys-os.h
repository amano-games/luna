#pragma once

#include "base/base-inc.h"
#include "sys/sys-defs.h"

// Platform hooks for optional hosts (Sokol helper today; native later).
// Each platform TU implements these, then includes its host helper.

void sys_os_boot_env(str8 exe_path, struct alloc scratch);
void sys_os_process_info_fill(struct sys_process_info *info, struct alloc alloc, struct alloc scratch);
str8 sys_os_get_current_path(struct alloc alloc, struct alloc scratch);
b32 sys_os_make_dir(str8 path, struct alloc scratch);
u32 sys_os_epoch_2000(u32 *milliseconds);
