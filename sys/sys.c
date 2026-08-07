// Helpers, Implemented Once (shared game host loop / timing / mem split)

#include "sys/sys.h"
#include "base/mem.h"
#include "base/prof.h"
#include "sys/sys-font.h"
#include "base/log.h"
#include "base/dbg.h"
#include "sys/sys-mem.h"

#if !defined(SYS_SHOW_FPS)
#define SYS_SHOW_FPS 1 // enable fps/ups counter
#endif

#define SYS_MEM_POISON_PATTERN 0xCD
#define SYS_LOG_LABEL          "sys"

struct sys_data SYS;
struct prof PROFILER;

struct app_mem
sys_init_mem(ssize permanent, ssize transient, ssize align, b32 clear)
{
	struct app_mem res      = {0};
	ssize mem_max           = SYS_MAX_MEM;
	struct sys_mem *sys_mem = &SYS.mem;
	ssize permanent_aligned = ALIGN_POW2(permanent, align);
	ssize transient_aligned = ALIGN_POW2(transient, align);
	ssize app_mem           = permanent_aligned + transient_aligned;
	struct alloc alloc      = sys_allocator();

	dbg_check(
		app_mem <= mem_max,
		SYS_LOG_LABEL,
		"Not enough sys memory | asked:%$$u available:%$$u missing:%$$u",
		(uint)app_mem,
		(uint)mem_max,
		(uint)(app_mem - mem_max));

	ssize debug_size = MAX(0, mem_max - permanent_aligned - transient_aligned);
	ssize mem_total  = permanent_aligned + transient_aligned + debug_size;

	log_info(SYS_LOG_LABEL, "Permanent: %$$u aligned:%$$u", (uint)permanent, (uint)permanent_aligned);
	log_info(SYS_LOG_LABEL, "Transient: %$$u aligned:%$$u", (uint)transient, (uint)transient_aligned);
	log_info(SYS_LOG_LABEL, "Debug    : %$$u", (uint)debug_size);
	log_info(SYS_LOG_LABEL, "Total    : %$$u/%$$u", (uint)mem_total, (uint)mem_max);
	log_info(SYS_LOG_LABEL, "Total    : %'u/%'u", (uint)mem_total, (uint)mem_max);

	sys_mem->app_mem.size   = mem_total;
	sys_mem->app_mem.buffer = alloc_size_aligned(alloc, sys_mem->app_mem.size, align, false);

	dbg_check(
		sys_mem->app_mem.buffer != NULL,
		SYS_LOG_LABEL,
		"Failed to reserve app memory %$$u: %p",
		(uint)sys_mem->app_mem.size,
		sys_mem->app_mem.buffer);

#if BUILD_DEBUG
	mset(sys_mem->app_mem.buffer, SYS_MEM_POISON_PATTERN, sys_mem->app_mem.size);
#endif

	res.permanent.size   = permanent_aligned;
	res.transient.size   = transient_aligned;
	res.debug.size       = debug_size;
	res.permanent.buffer = sys_mem->app_mem.buffer;
	res.transient.buffer = (u8 *)sys_mem->app_mem.buffer + res.permanent.size;
	res.debug.buffer     = (u8 *)sys_mem->app_mem.buffer + res.permanent.size + res.transient.size;
	res.is_initialized   = true;

	if(clear) {
		mclr(res.transient.buffer, res.transient.size);
		mclr(res.permanent.buffer, res.permanent.size);
		mclr(res.debug.buffer, res.debug.size);
	}
	return res;

error:
	if(sys_mem->app_mem.buffer != NULL) {
		sys_free(sys_mem->app_mem.buffer);
	}
	return (struct app_mem){0};
}

void
sys_ups_target_set(u32 value)
{
	dbg_assert(value > 0);
	SYS.timing.ups_target = value;
	SYS.timing.dt_us      = 1000000u / value; // 20.0 ms (50 UPS)
}

u32
sys_ups_target_get(void)
{
	return SYS.timing.ups_target;
}

void
sys_fps_target_set(u32 value)
{
	dbg_assert(value > 0);
	SYS.timing.fps_target   = value;
	SYS.timing.render_dt_us = 1000000u / value;
}

u32
sys_fps_target_get(void)
{
	return SYS.timing.fps_target;
}

u32
sys_dt_us_target_get(void)
{
	return SYS.timing.dt_us;
}

void
sys_dt_cap_us_set(u32 value)
{
	SYS.timing.dt_cap_us = value;
}

u32
sys_dt_cap_us_get(void)
{
	return SYS.timing.dt_cap_us;
}

void
sys_internal_init(void)
{
	sys_ups_target_set(SYS_DEFAULT_UPS);
	sys_dt_cap_us_set(SYS_DEFAULT_UPS_DT_CAP_US);

	SYS.last_time_us = sys_time_us();
	SYS.frame_buffer = sys_1bit_buffer();
#if defined(PROF)
	prof_ini();
#endif
	app_init(SYS_MAX_MEM);
}

// there are some frame skips when using the exact delta time and evaluating
// if an update tick should run (@50 FPS cap on hardware)
//
// https://medium.com/@tglaiel/how-to-make-your-game-run-at-60fps-24c61210fe75
i32
sys_internal_update(void)
{
	struct sys_data *sys = &SYS;
	u32 now              = sys_time_us();
	u32 time_delta       = now - sys->last_time_us;
	sys->last_time_us    = now;
	sys->timing.acc_us += time_delta;
	sys->timing.render_acc_us += time_delta;

#if defined(PROF)
	prof_upd(sys->prof_record_data);
#endif

	if(sys->timing.dt_cap_us < sys->timing.acc_us) {
		sys->timing.acc_us = sys->timing.dt_cap_us;
	}

#if SYS_SHOW_FPS
	u32 tu1 = sys_time_us();
#endif

	while(sys->timing.acc_us >= sys->timing.dt_us) {
		sys->timing.acc_us -= sys->timing.dt_us;
		sys->tick++;
		sys->timing.ups_counter++;
#if defined(PROF)
		prof_block_start("upd", PROF_ANCHOR_SYS_UPD);
#endif
		app_tick((f32)sys->timing.dt_us * 1e-6f);
#if defined(PROF)
		prof_block_end_internal();
#endif
	}

	b32 should_render = false;
	if(sys->timing.render_acc_us >= sys->timing.render_dt_us) {
		sys->timing.render_acc_us -= sys->timing.render_dt_us;
		should_render = true;
	}

#if SYS_SHOW_FPS
	u32 tu2 = sys_time_us();
	sys->timing.cpu_time_acc_us += tu2 - tu1;
#endif

	if(should_render) {
#if SYS_SHOW_FPS
		u32 tf1 = sys_time_us();
#endif
#if defined(PROF)
		prof_block_start("drw", PROF_ANCHOR_SYS_DRW);
#endif
		app_draw();
#if defined(PROF)
		prof_block_end_internal();
#endif
#if SYS_SHOW_FPS
		u32 tf2 = sys_time_us();
		sys->timing.fps_dt_acc_us += tf2 - tf1;
		sys->timing.fps_counter++;

		i32 fps_ft = 100 <= sys->timing.fps_avg_cpu_us ? 99 : sys->timing.fps_avg_cpu_us;
		i32 ups_ft = 100 <= sys->timing.ups_avg_cpu_us ? 99 : sys->timing.ups_avg_cpu_us;
		char fps[] = {
			'0' + (sys->timing.fps / 10),
			'0' + (sys->timing.fps % 10),
			'\0',
		};
		char ups[] = {
			'U',
			' ',
			'0' + (sys->timing.ups / 10),
			'0' + (sys->timing.ups % 10),
			' ',
			10 <= ups_ft ? '0' + (ups_ft / 10) % 10 : ' ',
			'0' + (ups_ft % 10),
			'\0',
		};
		sys_blit_text(&SYS, fps, 0, 29);
		// sys_blit_text(ups, 0, 1);
#endif
	}

#if SYS_SHOW_FPS
	sys->timing.stats_time_acc_us += time_delta;
	if(1000000u <= sys->timing.stats_time_acc_us) {
		sys->timing.stats_time_acc_us -= 1000000u;
		sys->timing.fps         = sys->timing.fps_counter;
		sys->timing.ups         = sys->timing.ups_counter;
		sys->timing.ups_counter = 0;
		sys->timing.fps_counter = 0;
		if(0 < sys->timing.ups) {
			sys->timing.ups_avg_cpu_us = (u16)((sys->timing.cpu_time_acc_us) / sys->timing.ups);
		} else {
			sys->timing.ups_avg_cpu_us = U16_MAX;
		}
		if(0 < sys->timing.fps) {
			sys->timing.fps_avg_cpu_us = (u16)((sys->timing.fps_dt_acc_us) / sys->timing.fps);
		} else {
			sys->timing.fps_avg_cpu_us = U16_MAX;
		}
		sys->timing.fps_dt_acc_us   = 0;
		sys->timing.cpu_time_acc_us = 0;
	}
#endif
	return should_render;
}

void
sys_internal_audio(i16 *lbuf, i16 *rbuf, i32 len)
{
#if SYS_SHOW_FPS
	u32 tu1 = sys_time_us();
#endif
	app_audio(lbuf, rbuf, len);
#if SYS_SHOW_FPS
	u32 tu2 = sys_time_us();
	SYS.timing.cpu_time_acc_us += tu2 - tu1;
#endif
}

void
sys_internal_close(void)
{
	app_close();
	sys_free(SYS.mem.app_mem.buffer);
}

void
sys_internal_pause(void)
{
	app_pause();
}

void
sys_internal_resume(void)
{
	app_resume();
}

void
sys_internal_stream_start(void)
{
	app_stream_start();
}

void
sys_internal_stream_end(void)
{
	app_stream_end();
}

void
sys_blit_text(struct sys_data *sys, char *str, i32 tile_x, i32 tile_y)
{
	u8 *fb = (u8 *)sys->frame_buffer;
	i32 x  = tile_x;
	for(char *c = str; *c != '\0'; c++) {
		i32 cx = ((i32)*c & 31);
		i32 cy = ((i32)*c >> 5) << 3;
		for(i32 n = 0; n < 8; n++) {
			fb[x + ((tile_y << 3) + n) * SYS_DISPLAY_WBYTES] =
				((u8 *)SYS_CONSOLE_FONT)[cx + ((cy + n) << 5)];
		}
		x++;
	}
}

void
sys_prof_pause(void)
{
	SYS.prof_record_data = false;
}

void
sys_prof_resume(void)
{
	SYS.prof_record_data = true;
}
