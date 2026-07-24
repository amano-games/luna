#pragma once

#include "base/base_inc.h"
#include "base/mem.h"
#include "lib/tex/tex.h"
#include "sys/sys-defs.h"

// App hooks
void app_init(usize mem_max);
void app_tick(f32 dt);
void app_draw(void);
void app_audio(i16 *lbuf, i16 *rbuf, i32 len);
void app_close(void);
void app_pause(void);
void app_resume(void);
void app_stream_start(void);
void app_stream_end(void);

struct app_mem sys_init_mem(usize permanent, usize transient, usize debug, b32 clear);

void sys_ups_target_set(u32 value);
u32 sys_ups_target_get(void);
void sys_fps_target_set(u32 value);
u32 sys_fps_target_get(void);
u32 sys_dt_us_target_get(void);
void sys_dt_cap_us_set(u32 value);
u32 sys_dt_cap_us_get(void);

void sys_internal_init(void);
i32 sys_internal_update(void);
void sys_internal_audio(i16 *lbuf, i16 *rbuf, i32 len);
void sys_internal_close(void);
void sys_internal_pause(void);
void sys_internal_resume(void);
void sys_internal_stream_start(void);
void sys_internal_stream_end(void);

void sys_blit_text(struct sys_data *sys, char *str, i32 tile_x, i32 tile_y);
void sys_prof_pause(void);
void sys_prof_resume(void);

// @per_backend_impl Memory
void *sys_alloc(void *ptr, ssize size, ssize align);
struct alloc sys_allocator(void);
void sys_free(void *ptr);

// @per_backend_impl Time
f32 sys_time_elapsed(void);
void sys_time_elapsed_reset(void);
u32 sys_time_ms(void);
u32 sys_time_us(void);
u32 sys_epoch_2000(u32 *milliseconds);

// @per_backend_impl Display (1-bit framebuffer)
void sys_1bit_invert(b32 i);
void *sys_1bit_buffer(void);
v4 sys_color_v4_get(enum gfx_col color);
void sys_color_v4_set(enum gfx_col color, v4 value);
u32 sys_color_u32_get(enum gfx_col color);
void sys_color_u32_set(enum gfx_col color, u32 value);

// @per_backend_impl Audio
// TODO: Playdate use one of the options to get volume
// float playdate->system->getVolume(void)
// float playdate->system->getSystemVolume(void)

#if SYS_BACKEND_PLAYDATE
#define sys_audio_set_volume(V) ((void)0)
#define sys_audio_get_volume()  (1.f)
#define sys_audio_lock()        ((void)0)
#define sys_audio_unlock()      ((void)0)
#else
void sys_audio_set_volume(f32 vol);
f32 sys_audio_get_volume(void);
void sys_audio_lock(void);
void sys_audio_unlock(void);
#endif

// @per_backend_impl System menu / host chrome

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

// Playdate-only (not part of portable contract; Phase 4 may fold present into sys_*)
#if SYS_BACKEND_PLAYDATE
b32 sys_pd_reduce_flicker(void);
f32 sys_pd_crank_deg(void);
void sys_pd_update_rows(i32 from_incl, i32 to_incl);
#endif
