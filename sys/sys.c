#include "sys.h"
#include "mem-arena.h"
#include "sys-io.h"
#include "sys-utils.h"
#include "sys-backend.h"
#include "sys-font.h"
#include "sys-log.h"
#include "sys-assert.h"

#if !defined(SYS_SHOW_FPS)
#define SYS_SHOW_FPS 1 // enable fps/ups counter
#endif

#define SYS_FIXED_DT    .0200f // 1 /  50
#define SYS_DT_MAX      .0600f // 3 /  50fps
#define SYS_UPS_DT_TEST .0195f // 1 / ~51

static struct {
	void *menu_items[8];
	f32 last_time;
	u32 tick;
	f32 ups_time_accumulator;
	struct sys_mem mem;
#if SYS_SHOW_FPS
	f32 fps_time_acc;
	int fps_counter;
	int fps; // updates per second
#endif
} SYS;

void
app_parse_metadata(struct app_metadata *app_metadata, struct marena *scratch)
{
	{
		void *f = sys_file_open("pdxinfo", SYS_FILE_R);
		if(f != NULL) {
			struct sys_file_stats stats = sys_fstats("pdxinfo");
			char *data                  = marena_alloc(scratch, stats.size);
			sys_file_read(f, (void *)data, stats.size);
			sys_printf("%s", data);
		}
	}
}

void
sys_init(void)
{
	usize max_mem                    = SYS_MAX_MEM;
	struct sys_mem *sys_mem          = &SYS.mem;
	struct app_mem app_mem           = {0};
	struct app_metadata app_metadata = {0};

	app_mem.permanent.size = MMEGABYTE(2.5);
	app_mem.transient.size = MMEGABYTE(2.5);
	app_mem.debug.size     = 0;

#ifdef DEBUG
	app_mem.debug.size = max_mem - app_mem.permanent.size - app_mem.transient.size;
#endif

	assert((app_mem.permanent.size + app_mem.transient.size + app_mem.debug.size) <= max_mem);
	sys_mem->app_mem.size   = app_mem.permanent.size + app_mem.transient.size + app_mem.debug.size;
	sys_mem->app_mem.buffer = backend_alloc(sys_mem->app_mem.buffer, sys_mem->app_mem.size);
	if(sys_mem->app_mem.buffer == NULL) {
		log_error("Sys", "Failed to reserve app memory %u kb", (uint)sys_mem->app_mem.size / 1024);
	}
	app_mem.permanent.buffer = sys_mem->app_mem.buffer;
	app_mem.transient.buffer = (u8 *)sys_mem->app_mem.buffer + app_mem.permanent.size;
	app_mem.debug.buffer     = (u8 *)sys_mem->app_mem.buffer + app_mem.permanent.size + app_mem.transient.size;
	app_mem.is_initialized   = true;
	log_info("Sys", "App memory %u kb", (uint)sys_mem->app_mem.size / 1024);

	struct marena scratch = {0};
	marena_init(&scratch, app_mem.permanent.buffer, MMEGABYTE(1));

	app_parse_metadata(&app_metadata, &scratch);
	marena_reset(&scratch);

	memset(app_mem.permanent.buffer, 0, MMEGABYTE(1));
	app_init(app_mem);
}

// there are some frame skips when using the exact delta time and evaluating
// if an update tick should run (@50 FPS cap on hardware)
//
// https://medium.com/@tglaiel/how-to-make-your-game-run-at-60fps-24c61210fe75
int
sys_tick(void *arg)
{
	f32 time       = backend_seconds();
	f32 time_delta = time - SYS.last_time;
	SYS.last_time  = backend_seconds();
	SYS.ups_time_accumulator += time_delta;

	SYS.ups_time_accumulator = MIN(SYS_DT_MAX, SYS.ups_time_accumulator);

	int n_upd = 0;
	while(SYS.ups_time_accumulator > SYS_UPS_DT_TEST) {
		f32 dt = SYS_FIXED_DT;

		app_tick(dt);
		SYS.ups_time_accumulator -= SYS_FIXED_DT;

		if(SYS.ups_time_accumulator < 0) {
			SYS.ups_time_accumulator = 0;
		}

		n_upd++;
		SYS.tick++;
	}

	if(n_upd > 0) {
		app_draw();
#if SYS_SHOW_FPS
		char fps[2] = {'0' + (SYS.fps / 10), '0' + (SYS.fps % 10)};

		u8 *fb = (u8 *)backend_framebuffer();
		for(int k = 0; k <= 1; k++) {
			int cx = ((int)fps[k] & 31);
			int cy = ((int)fps[k] >> 5) << 3;

			for(int n = 0; n < 8; n++) {
				int i = k + n * SYS_DISPLAY_WBYTES;
				int j = cx + ((cy + n) << 5);
				fb[i] = ((u8 *)SYS_CONSOLEFONT)[j];
			}
		}
		sys_display_update_rows(0, 239);
		SYS.fps_counter++;
#endif
	}

#if SYS_SHOW_FPS
	SYS.fps_time_acc += time_delta;
	if(1.0f <= SYS.fps_time_acc) {
		SYS.fps_time_acc -= 1.f;
		SYS.fps         = SYS.fps_counter;
		SYS.fps_counter = 0;
	}
#endif

	return (0 < n_upd);
}

void
sys_close(void)
{
	app_close();
	backend_free(SYS.mem.app_mem.buffer);
}

void
sys_pause(void)
{
	app_pause();
}

void
sys_resume(void)
{
	app_resume();
}

f32
sys_seconds(void)
{
	f32 r = backend_seconds();
	return r;
}

struct sys_display
sys_display(void)
{
	struct sys_display s;
	s.px    = backend_framebuffer();
	s.w     = SYS_DISPLAY_W;
	s.h     = SYS_DISPLAY_H;
	s.wbyte = SYS_DISPLAY_WBYTES;
	s.wword = SYS_DISPLAY_WWORDS;

	return s;
}

void
sys_display_update_rows(int a, int b)
{
	assert(0 <= a && b < SYS_DISPLAY_H);
	backend_display_row_updated(a, b);
}

void
sys_debug_draw(struct debug_shape *shapes, int count)
{
	backend_draw_debug_shape(shapes, count);
}

void
sys_menu_item_add(int id, const char *title, void (*callback)(void *arg), void *arg)
{
	void *item         = backend_menu_item_add(title, callback, arg);
	SYS.menu_items[id] = item;
}

void
sys_menu_checkmark_add(int id, const char *title, int val, void (*callback)(void *arg), void *arg)
{
	void *item         = backend_menu_checkmark_add(title, val, callback, arg);
	SYS.menu_items[id] = item;
}

void
sys_menu_options_add(int id, const char *title, const char **options, int count, void (*callback)(void *arg), void *arg)
{
	void *item         = backend_menu_options_add(title, options, count, callback, arg);
	SYS.menu_items[id] = item;
}

int
sys_menu_value(int id)
{
	return backend_menu_value(SYS.menu_items[id]);
}

void
sys_menu_clr(void)
{
	backend_menu_clr();
	memset(SYS.menu_items, 0, sizeof(SYS.menu_items));
}
