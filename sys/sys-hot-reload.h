#pragma once

#include "base/base-inc.h"
#include "sys/sys-io.h"
#include "sys/sys-os.h"

struct sys_loaded_code {
	str8 so_path;                // build/linux/game.so (next to the exe)
	str8 lock_path;              // build/linux/game.lock, present while the .so builds
	str8 tmp_prefix;             // build/linux/obj/game-hot-, + idx + so_path extension
	u32 tmp_idx;                 // rotates 0..1023 like HMH TempDLLNumber
	sys_lib lib;                 // never closed, old images may still be referenced
	struct sys_file_props props; // so_path props at load, like HMH DLLLastWriteTime
};

b32 sys_code_props_changed(struct sys_file_props loaded, struct sys_file_props current);
struct app_code sys_code_load(struct sys_loaded_code *c);
