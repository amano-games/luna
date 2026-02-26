#include "sys.h"
#include "base/prof.h"
#include "sys-font.h"
#include "base/log.h"
#include "base/dbg.h"

#if !defined(SYS_SHOW_FPS)
#define SYS_SHOW_FPS 1 // enable fps/ups counter
#endif

#define SYS_MEM_POISON_PATTERN 0xCD
#define SYS_LOG_LABEL          "sys"

struct sys_data SYS;

struct app_mem
sys_init_mem(usize permanent, usize transient, usize debug, b32 clear)
{
	struct app_mem res      = {0};
	usize mem_max           = SYS_MAX_MEM;
	struct sys_mem *sys_mem = &SYS.mem;
	usize mem_total         = permanent + transient + debug;

	log_info(SYS_LOG_LABEL, "Permanent: %$$u", (uint)permanent);
	log_info(SYS_LOG_LABEL, "Transient: %$$u", (uint)transient);
	log_info(SYS_LOG_LABEL, "Debug    : %$$u", (uint)debug);
	log_info(SYS_LOG_LABEL, "Total    : %$$u/%$$u", (uint)mem_total, (uint)mem_max);
	log_info(SYS_LOG_LABEL, "Total    : %'u/%'u", (uint)mem_total, (uint)mem_max);
	dbg_check(
		mem_total <= mem_max,
		SYS_LOG_LABEL,
		"Not enough sys memory | asked:%$$u available:%$$u missing:%$$u",
		(uint)mem_total,
		(uint)mem_max,
		(uint)((mem_total - mem_max)));

	sys_mem->app_mem.size   = mem_total;
	sys_mem->app_mem.buffer = sys_alloc(sys_mem->app_mem.buffer, sys_mem->app_mem.size, 4);

	dbg_check(
		sys_mem->app_mem.buffer != NULL,
		SYS_LOG_LABEL,
		"Failed to reserve app memory %$$u: %p",
		(uint)sys_mem->app_mem.size,
		sys_mem->app_mem.buffer);

#if defined(DEBUG)
	mset(sys_mem->app_mem.buffer, SYS_MEM_POISON_PATTERN, sys_mem->app_mem.size);
#endif

	res.permanent.size   = permanent;
	res.transient.size   = transient;
	res.debug.size       = debug;
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
sys_internal_init(void)
{
	SYS.fps          = SYS_UPS;
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
	u32 now          = sys_time_us();
	u32 time_delta   = now - SYS.last_time_us;
	SYS.last_time_us = now;
	SYS.ups_time_acc_us += time_delta;

#if defined(PROF)
	prof_upd(true);
#endif

	if(SYS_UPS_DT_CAP_US < SYS.ups_time_acc_us) {
		SYS.ups_time_acc_us = SYS_UPS_DT_CAP_US;
	}

#if SYS_SHOW_FPS
	u32 tu1 = sys_time_us();
#endif

	b32 updated = 0;

	while(SYS.ups_time_acc_us >= SYS_UPS_DT_US) {
		SYS.ups_time_acc_us -= SYS_UPS_DT_US;
		SYS.tick++;
		SYS.ups_counter++;
		updated = 1;
		prof_block("upd");
		app_tick((f32)SYS_UPS_DT_US * 1e-6f);
		prof_block_end();
	}

#if SYS_SHOW_FPS
	u32 tu2 = sys_time_us();
	SYS.ups_ft_acc_us += tu2 - tu1;
#endif

	if(updated) {
#if SYS_SHOW_FPS
		u32 tf1 = sys_time_us();

		prof_block("drw");
		app_draw();
		prof_block_end();

		u32 tf2 = sys_time_us();
		SYS.fps_ft_acc_us += tf2 - tf1;
		SYS.fps_counter++;

		i32 fps_ft = 100 <= SYS.fps_ft ? 99 : SYS.fps_ft;
		i32 ups_ft = 100 <= SYS.ups_ft ? 99 : SYS.ups_ft;
		char fps[] = {
			'0' + (SYS.fps / 10),
			'0' + (SYS.fps % 10),
			'\0',
		};
		char ups[] = {
			'U',
			' ',
			'0' + (SYS.ups / 10),
			'0' + (SYS.ups % 10),
			' ',
			10 <= ups_ft ? '0' + (ups_ft / 10) % 10 : ' ',
			'0' + (ups_ft % 10),
			'\0',
		};
		sys_blit_text(&SYS, fps, 0, 29);
		// sys_blit_text(ups, 0, 1);
#else
		app_draw();
#endif
	}

#if SYS_SHOW_FPS
	SYS.fps_time_acc_us += time_delta;
	if(1000000u <= SYS.fps_time_acc_us) {
		SYS.fps_time_acc_us -= 1000000u;
		SYS.fps         = SYS.fps_counter;
		SYS.ups         = SYS.ups_counter;
		SYS.ups_counter = 0;
		SYS.fps_counter = 0;
		if(0 < SYS.ups) {
			SYS.ups_ft = (u16)((SYS.ups_ft_acc_us) / SYS.ups);
		} else {
			SYS.ups_ft = U16_MAX;
		}
		if(0 < SYS.fps) {
			SYS.fps_ft = (u16)((SYS.fps_ft_acc_us) / SYS.fps);
		} else {
			SYS.fps_ft = U16_MAX;
		}
		SYS.fps_ft_acc_us = 0;
		SYS.ups_ft_acc_us = 0;
	}
#endif
	return updated;
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
	SYS.ups_ft_acc_us += tu2 - tu1;
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
