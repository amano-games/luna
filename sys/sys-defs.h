#pragma once

#include "base/base-inc.h"

#define SYS_DISPLAY_W      400
#define SYS_DISPLAY_H      240
#define SYS_DISPLAY_WBYTES 52
#define SYS_DISPLAY_WWORDS 13

#define SYS_DEFAULT_UPS           50
#define SYS_DEFAULT_UPS_DT_CAP_US 60000u // 60.0 ms clamp max elapsed seconds

#if BUILD_DEBUG
#define SYS_MAX_MEM MGIGABYTE(1)
#else
#define SYS_MAX_MEM MMEGABYTE(14)
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
	str8 binary_file_path;
	str8 binary_path;
	str8 base_path;
	str8 initial_path;
	str8 user_program_config_data_path;
	str8 user_program_cache_data_path;
	str8 user_program_logs_data_path;
	struct str8_list environment;
};

struct sys_timing {
	u16 ups_target; // target update rate
	u16 ups;        // updates updates/sec

	u16 fps_target; // target frames/sec
	u16 fps;        // mesured frames/sec
	u32 render_dt_us;

	u32 dt_us;     // microseconds per update
	u32 dt_cap_us; // max delta clamp

	u32 acc_us; // fixed timestep delta accumulator
	u32 render_acc_us;
	u32 stats_time_acc_us; // Accumulates time to measure 1 second window

	u32 cpu_time_acc_us; // acc us spent in upd + audio over the current stats window
	u32 fps_dt_acc_us;   // acc us spent in rendering (draw calls)

	u16 ups_counter; // update ticks executed in the current stats window(per second)
	u16 fps_counter; // frames drawn in the current stats window (per second)

	u32 ups_avg_cpu_us; // avg us per update tick (includes update + audio)
	u32 fps_avg_cpu_us; // avg us per frame (render/draw)
};

struct sys_data {
	void *frame_buffer;

	u32 tick;
	u32 last_time_us;

	struct sys_timing timing;

	b32 prof_record_data;
	struct sys_mem mem;
};

struct app_mem {
	struct mem_block permanent;
	struct mem_block transient;
	struct mem_block debug;

	bool is_initialized;
};
