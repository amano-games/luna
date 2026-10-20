#pragma once

#include "base/base-inc.h"
#include "sys/sys-io.h"
#include "sys/sys-os.h"

#if !defined(SYS_HOT_LIB_PATH)
#define SYS_HOT_LIB_PATH "game.so"
#endif
#if !defined(SYS_HOT_LOCK_PATH)
#define SYS_HOT_LOCK_PATH "game.lock"
#endif
#if !defined(SYS_HOT_TMP_PREFIX)
#define SYS_HOT_TMP_PREFIX "obj/game-hot-"
#endif

struct sys_loaded_code {
	str8 so_path;                // build/linux/game.so (next to the exe)
	str8 lock_path;              // build/linux/game.lock, present while the .so builds
	str8 tmp_prefix;             // build/linux/obj/game-hot-, + idx + so_path extension
	u32 tmp_idx;                 // rotates 0..1023 like HMH TempDLLNumber
	sys_lib lib;                 // never closed, old images may still be referenced
	struct sys_file_props props; // so_path props at load, like HMH DLLLastWriteTime
	u32 poll_last_us;            // sys_code_changed throttle
};

struct sys_loaded_code sys_code_ini(struct alloc alloc, str8 base_dir);
b32 sys_code_props_changed(struct sys_file_props loaded, struct sys_file_props current);
struct app_code sys_code_load(struct sys_loaded_code *c);
void sys_code_unload(struct sys_loaded_code *c, struct app_code *code);
b32 sys_code_changed(struct sys_loaded_code *c);
b32 sys_code_reload(struct sys_loaded_code *c, struct app_code *code);
