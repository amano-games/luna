#include "sys.h"
#include "sys-utils.h"
#include "sys-font.h"
#include "sys-log.h"
#include "sys-assert.h"

#if !defined(SYS_SHOW_FPS)
#define SYS_SHOW_FPS 1 // enable fps/ups counter
#endif

struct sys_data {
	void *frame_buffer;
	u32 tick;
	f32 last_time;
	f32 ups_time_acc;
	f32 fps_time_acc;
	f32 ups_ft_acc;
	f32 fps_ft_acc;
	u16 fps_counter;
	u16 ups_counter;
	u16 fps; // rendered frames per second
	u16 ups; // updates per second
	u16 ups_ft;
	u16 fps_ft;
	void *menu_items[8];
	struct sys_mem mem;
};

struct sys_data SYS;

void
sys_internal_init(void)
{
	usize max_mem           = SYS_MAX_MEM;
	struct sys_mem *sys_mem = &SYS.mem;
	struct app_mem app_mem  = {0};
	SYS.fps                 = SYS_UPS;
	SYS.last_time           = sys_seconds();
	SYS.frame_buffer        = sys_1bit_buffer();

	app_mem.permanent.size = MMEGABYTE(2.5);
	app_mem.transient.size = MMEGABYTE(2.5);
	app_mem.debug.size     = 0;

#ifdef DEBUG
	app_mem.debug.size = max_mem - app_mem.permanent.size - app_mem.transient.size;
#endif

	assert((app_mem.permanent.size + app_mem.transient.size + app_mem.debug.size) <= max_mem);
	sys_mem->app_mem.size   = app_mem.permanent.size + app_mem.transient.size + app_mem.debug.size;
	sys_mem->app_mem.buffer = sys_alloc(sys_mem->app_mem.buffer, sys_mem->app_mem.size);
	if(sys_mem->app_mem.buffer == NULL) {
		log_error("Sys", "Failed to reserve app memory %u kb", (uint)sys_mem->app_mem.size / 1024);
	}
	app_mem.permanent.buffer = sys_mem->app_mem.buffer;
	app_mem.transient.buffer = (u8 *)sys_mem->app_mem.buffer + app_mem.permanent.size;
	app_mem.debug.buffer     = (u8 *)sys_mem->app_mem.buffer + app_mem.permanent.size + app_mem.transient.size;

	// memset(app_mem.transient.buffer, 0, app_mem.transient.size);
	// memset(app_mem.permanent.buffer, 0, app_mem.permanent.size);
	// memset(app_mem.debug.buffer, 0, app_mem.debug.size);

	app_mem.is_initialized = true;

	log_info("Sys", "App memory %u kb", (uint)sys_mem->app_mem.size / 1024);

	app_init(app_mem);
}

// there are some frame skips when using the exact delta time and evaluating
// if an update tick should run (@50 FPS cap on hardware)
//
// https://medium.com/@tglaiel/how-to-make-your-game-run-at-60fps-24c61210fe75
i32
sys_internal_update(void)
{
	f32 time       = sys_seconds();
	f32 time_delta = time - SYS.last_time;
	SYS.last_time  = time;
	SYS.ups_time_acc += time_delta;

	if(SYS_UPS_DT_CAP < SYS.ups_time_acc) {
		SYS.ups_time_acc = SYS_UPS_DT_CAP;
	}

#if SYS_SHOW_FPS
	f32 tu1 = sys_seconds();
#endif

	bool32 updated = 0;

	while(SYS_UPS_DT_TEST <= SYS.ups_time_acc) {
		SYS.ups_time_acc -= SYS_UPS_DT;
		SYS.tick++;
		SYS.ups_counter++;
		updated = 1;
		app_tick(SYS_UPS_DT);
	}
#if SYS_SHOW_FPS
	f32 tu2 = sys_seconds();
	SYS.ups_ft_acc += tu2 - tu1;
#endif

	if(updated) {
#if SYS_SHOW_FPS
		f32 tf1 = sys_seconds();
		app_draw();
		f32 tf2 = sys_seconds();
		SYS.fps_ft_acc += tf2 - tf1;
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
		sys_blit_text(fps, 0, 29);
		// sys_blit_text(ups, 0, 1);
#else
		app_draw();
#endif
	}

#if SYS_SHOW_FPS
	SYS.fps_time_acc += time_delta;
	if(1.f <= SYS.fps_time_acc) {
		SYS.fps_time_acc -= 1.f;
		SYS.fps         = SYS.fps_counter;
		SYS.ups         = SYS.ups_counter;
		SYS.ups_counter = 0;
		SYS.fps_counter = 0;
		if(0 < SYS.ups) {
			SYS.ups_ft = (i32)(SYS.ups_ft_acc * 1000.5f) / SYS.ups;
		} else {
			SYS.ups_ft = U16_MAX;
		}
		if(0 < SYS.fps) {
			SYS.fps_ft = (i32)(SYS.fps_ft_acc * 1000.5f) / SYS.fps;
		} else {
			SYS.fps_ft = U16_MAX;
		}
		SYS.fps_ft_acc = 0.f;
		SYS.ups_ft_acc = 0.f;
	}
#endif
	return updated;
}

void
sys_internal_audio(i16 *lbuf, i16 *rbuf, i32 len)
{
#if SYS_SHOW_FPS
	f32 tu1 = sys_seconds();
#endif
	app_audio(lbuf, rbuf, len);
#if SYS_SHOW_FPS
	f32 tu2 = sys_seconds();
	SYS.ups_ft_acc += tu2 - tu1;
#endif
}

void
sys_blit_text(char *str, i32 tile_x, i32 tile_y)
{
	u8 *fb = (u8 *)SYS.frame_buffer;
	i32 i  = tile_x;
	for(char *c = str; *c != '\0'; c++) {
		i32 cx = ((i32)*c & 31);
		i32 cy = ((i32)*c >> 5) << 3;
		for(i32 n = 0; n < 8; n++) {
			fb[i + ((tile_y << 3) + n) * SYS_DISPLAY_WBYTES] =
				((u8 *)SYS_CONSOLE_FONT)[cx + ((cy + n) << 5)];
		}
		i++;
	}
}

u32
sys_time(void)
{
	return SYS.tick;
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
