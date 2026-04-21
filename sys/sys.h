#pragma once

#include "base/types.h"
#include "base/mem.h"
#include "lib/tex/tex.h"
#include "sys/sys-defs.h"

#if defined(BACKEND_SOKOL)
#include "sys-sokol.h"
#elif defined(BACKEND_PD)
#include "sys-pd.h"
#else
#include "sys-cli.h"
#endif

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
void app_stream_start(void);
void app_stream_end(void);

void *sys_alloc(void *ptr, ssize size, ssize align);
struct alloc sys_allocator(void);
void sys_free(void *ptr);

f32 sys_time_elapsed(void);
void sys_time_elapsed_reset(void);

u32 sys_time_ms(void);
u32 sys_time_us(void);

u32 sys_epoch_2000(u32 *milliseconds);

void sys_1bit_invert(b32 i);
void *sys_1bit_buffer(void);

void sys_accelerometer_set(b32 enabled);
void sys_accelerometer(f32 *x, f32 *y, f32 *z);
struct app_mem sys_init_mem(usize permanent, usize transient, usize debug, b32 clear);

void sys_ups_target_set(u32 value);
u32 sys_ups_target_get(void);

void sys_fps_target_set(u32 value);
u32 sys_fps_target_get(void);

u32 sys_dt_us_target_get(void);

void sys_dt_cap_us_set(u32 value);
u32 sys_dt_cap_us_get(void);

v4 sys_color_v4_get(enum gfx_col color);
void sys_color_v4_set(enum gfx_col color, v4 value);

u32 sys_color_u32_get(enum gfx_col color);
void sys_color_u32_set(enum gfx_col color, u32 value);

void sys_internal_init(void);
i32 sys_internal_update(void);
void sys_internal_audio(i16 *lbuf, i16 *rbuf, i32 len);
void sys_internal_close(void);
void sys_internal_pause(void);
void sys_internal_resume(void);
void sys_internal_stream_start(void);
void sys_internal_stream_end(void);

// TODO: Should we do this only on playdate?
i32 sys_menu_item_add(const char *title, void (*callback)(void *arg), void *arg);
i32 sys_menu_checkmark_add(const char *title, int val, void (*callback)(void *arg), void *arg);
i32 sys_menu_options_add(const char *title, const char **options, int count, void (*callback)(void *arg), void *arg);
int sys_menu_value(int id);
void sys_menu_item_remove(int id);
void sys_menu_clr(void);
void sys_set_menu_image(struct tex tex, i32 x_offset);
void sys_set_auto_lock_disabled(int disabled);
void sys_set_app_name(str8 value);
str8 sys_get_current_path(struct alloc alloc);
struct sys_process_info sys_process_info(void);

void sys_blit_text(struct sys_data *sys, char *str, i32 tile_x, i32 tile_y);

void sys_prof_pause(void);
void sys_prof_resume(void);
