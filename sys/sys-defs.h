#pragma once

#include "base/types.h"

#define SYS_UPS            50
#define SYS_DISPLAY_W      400
#define SYS_DISPLAY_H      240
#define SYS_DISPLAY_WBYTES 52
#define SYS_DISPLAY_WWORDS 13

#define SYS_UPS_DT_US      20000u // 20.0 ms (50 UPS) elapsed seconds per update step (1/UPS)
#define SYS_UPS_DT_TEST_US 19500u // 19.5 ms tolerance elapsed seconds required to run a tick - improves frame skips at max FPS
#define SYS_UPS_DT_CAP_US  60000u // 60.0 ms clamp max elapsed seconds

#if defined BACKEND_PD
#define SYS_ACCELEROMETER_SUPPORT 1
#else
#define SYS_ACCELEROMETER_SUPPORT 0
#endif

#if defined(DEBUG)
#define SYS_MAX_MEM MGIGABYTE(1)
#else
#define SYS_MAX_MEM MMEGABYTE(15)
#endif

struct mem_block {
	usize size;
	alignas(8) void *buffer;
};

struct sys_mem {
	struct mem_block app_mem;
};

struct sys_process_info {
	u32 pid;
	str8 exe_path;
	str8 module_path;
	str8 base_path;
	str8 initial_path;
	str8 data_path;
	str8 environment;
};

struct sys_data {
	void *frame_buffer;

	u32 tick;
	u32 last_time_us;

	u32 ups_time_acc_us; // fixed timestep delta accumulator
	u32 fps_time_acc_us;

	u32 ups_ft_acc_us;
	u32 fps_ft_acc_us;

	u16 fps_counter;
	u16 ups_counter;

	u16 fps; // rendered frames per second
	u16 ups; // updates per second
	u16 ups_ft;
	u16 fps_ft;
	struct sys_mem mem;
};

struct app_mem {
	struct mem_block permanent;
	struct mem_block transient;
	struct mem_block debug;

	bool is_initialized;
};
