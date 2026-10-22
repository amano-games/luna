#pragma once

#include "base/base-inc.h"
#include "lib/fnt/fnt-defs.h"
#include "sys/sys-app-proc.h"

#define SYS_PD_DISPLAY_W 400
#define SYS_PD_DISPLAY_H 240

#define SYS_SHOW_FPS_NONE 0 // compile overlay out
#define SYS_SHOW_FPS_FPS  1 // two-digit FPS
#define SYS_SHOW_FPS_FULL 2 // FPS + UPS + miss + cpu ms

#if !defined(SYS_HOT_RELOAD)
#define SYS_HOT_RELOAD 0
#endif

// The Playdate panel cannot scan out faster than 50 fps
// 50 is both the update rate and the render rate ceiling.
#define SYS_DEFAULT_UPS 50
#define SYS_DEFAULT_FPS 50

#define SYS_DEFAULT_UPS_DT_CAP_TICKS 2
#define SYS_DEFAULT_UPS_DT_CAP_US    (SYS_DEFAULT_UPS_DT_CAP_TICKS * (1000000u / SYS_DEFAULT_UPS))

dbg_static_assert(SYS_DEFAULT_UPS <= 50, sys_ups_within_panel_ceiling);
dbg_static_assert(SYS_DEFAULT_FPS <= SYS_DEFAULT_UPS, sys_fps_not_above_ups);

#if BUILD_DEBUG
#define SYS_MAX_MEM MGIGABYTE(1)
#elif OS_PLAYDATE
#define SYS_MAX_MEM MMEGABYTE(14)
#else
#define SYS_MAX_MEM MMEGABYTE(64)
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
	str8 user_program_data_path;
	str8 user_program_cache_data_path;
	str8 user_program_logs_data_path;
	struct str8_list environment;
};

struct sys_timing {
	u16 ups_target; // target update rate
	u16 ups;        // updates updates/sec

	u16 fps_target; // target frames/sec
	u16 fps;        // mesured frames/sec
	u64 render_dt_us;

	u64 dt_us;     // microseconds per update
	u64 dt_cap_us; // max delta clamp

	u64 acc_us; // fixed timestep delta accumulator
	u64 render_acc_us;
	u64 stats_time_acc_us; // Accumulates time to measure 1 second window

	u64 cpu_time_acc_us; // acc us spent in upd + audio over the current stats window
	u64 drw_dt_acc_us;   // acc us spent in rendering (draw calls)

	u16 ups_counter;  // update ticks executed in the current stats window(per second)
	u16 drw_counter;  // frames drawn in the current stats window (per second)
	u16 miss;         // callbacks in the last window that ran != 1 tick
	u16 miss_counter; // current window

	u64 ups_avg_cpu_us; // avg us per update tick (includes update + audio)
	u64 drw_avg_cpu_us; // avg us per frame (render/draw)
};

#define APP_INIT(name)  void name(usize mem_max)
#define APP_TICK(name)  void name(f32 dt)
#define APP_DRAW(name)  void name(void)
#define APP_AUDIO(name) void name(i16 *lbuf, i16 *rbuf, i32 len)
#define APP_VOID(name)  void name(void)

typedef APP_INIT(app_init_fn);
typedef APP_TICK(app_tick_fn);
typedef APP_DRAW(app_draw_fn);
typedef APP_AUDIO(app_audio_fn);
typedef APP_VOID(app_void_fn);

struct app_code {
	b32 is_valid;
	union {
		app_void_fn *procs[APP_PROC_NUM_COUNT];
		struct {
			app_void_fn *none;
			app_init_fn *init;
			app_tick_fn *tick;
			app_draw_fn *draw;
			app_audio_fn *audio;
			app_void_fn *close;
			app_void_fn *pause;
			app_void_fn *resume;
			app_void_fn *stream_start;
			app_void_fn *stream_end;
		};
	};
};

struct sys_prof_state {
	b16 record;
	b16 save;
	b32 quit;
	struct alloc scratch;
};

struct sys_data {
	b32 initialized;
	struct app_code app;
	struct fnt fnt_mono;
	void *frame_buffer;

	u32 tick;
	u64 last_time_us;

	struct sys_timing timing;

	struct sys_prof_state prof_state;
	struct sys_mem mem;
};

struct app_mem {
	struct mem_block permanent;
	struct mem_block transient;
	struct mem_block debug;

	bool is_initialized;
};
