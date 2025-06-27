#pragma once

#include "sys-types.h"
#include "mem.h"

#define SYS_UPS            50
#define SYS_DISPLAY_W      400
#define SYS_DISPLAY_H      240
#define SYS_DISPLAY_WBYTES 52
#define SYS_DISPLAY_WWORDS 13

#define SYS_UPS_DT      0.0200f // elapsed seconds per update step (1/UPS)
#define SYS_UPS_DT_TEST 0.0195f // elapsed seconds required to run a tick - improves frame skips at max FPS
#define SYS_UPS_DT_CAP  0.0600f // max elapsed seconds

#if defined BACKEND_PD
#define SYS_ACCELEROMETER_SUPPORT 1
#else
#define SYS_ACCELEROMETER_SUPPORT 0
#endif

#if defined(DEBUG)
// #define SYS_MAX_MEM MMEGABYTE(100)
#define SYS_MAX_MEM MMEGABYTE(7.0)
#else
#define SYS_MAX_MEM MMEGABYTE(7.0)
#endif

#if defined(BACKEND_SOKOL)
#include "sys-sokol.h"
#elif defined(BACKEND_PD)
#include "sys-pd.h"
#else
#include "sys-cli.h"
#endif

struct mem_block {
	usize size;
	alignas(8) void *buffer;
};

struct sys_mem {
	struct mem_block app_mem;
};

struct app_mem {
	struct mem_block permanent;
	struct mem_block transient;
	struct mem_block debug;

	bool is_initialized;
};

#if defined BACKEND_PD
#define sys_audio_set_volume(V)
#define sys_audio_get_volume() 1.f
#define sys_audio_lock()
#define sys_audio_unlock()
#else
void sys_audio_set_volume(f32 vol);
f32 sys_audio_get_volume(void);
void sys_audio_lock(void);
void sys_audio_unlock(void);
#endif

void app_init(usize mem_max);
void app_tick(f32 dt);
void app_draw(void);
void app_audio(i16 *lbuf, i16 *rbuf, i32 len);
void app_close(void);
void app_pause(void);
void app_resume(void);

void *sys_alloc(void *ptr, usize size);
void sys_free(void *ptr);

void sys_blit_text(char *str, i32 tile_x, i32 tile_y);
f32 sys_seconds(void);
u32 sys_time(void);
u32 sys_epoch(u32 *milliseconds);

void sys_1bit_invert(bool32 i);
void *sys_1bit_buffer(void);
void *sys_1bit_menu_buffer(void);

void sys_accelerometer_set(bool32 enabled);
void sys_accelerometer(f32 *x, f32 *y, f32 *z);
struct app_mem sys_init_mem(usize permanent, usize transient, usize debug, bool32 clear);

void sys_internal_init(void);
i32 sys_internal_update(void);
void sys_internal_audio(i16 *lbuf, i16 *rbuf, i32 len);
void sys_internal_close(void);
void sys_internal_pause(void);
void sys_internal_resume(void);

// TODO: Should we do this only on playdate?
void sys_menu_item_add(int id, const char *title, void (*callback)(void *arg), void *arg);
void sys_menu_checkmark_add(int id, const char *title, int val, void (*callback)(void *arg), void *arg);
void sys_menu_options_add(int id, const char *title, const char **options, int count, void (*callback)(void *arg), void *arg);
int sys_menu_value(int id);
void sys_menu_clr(void);
void sys_set_menu_image(void *px, int h, int wbyte, i32 x_offset);
