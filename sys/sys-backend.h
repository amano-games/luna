#pragma once

#include "sys-types.h"
#include "sys-debug.h"

#include "str.h"

struct exe_path {
	str8 path;
	str8 dirname;
};

void sys_init(void);
int sys_tick(void *arg);
void sys_close(void);
void sys_pause(void);
void sys_resume(void);

void backend_display_row_updated(int a, int b);
f32 backend_seconds(void);
u32 *backend_framebuffer(void);

void *backend_alloc(void *ptr, usize size);
void backend_free(void *ptr);
i32 backend_parse_string(const char *str, const char *format, ...);
void backend_log(const char *tag, u32 log_level, u32 log_item, const char *msg, uint32_t line_nr, const char *filename);

// IO
struct sys_file_stats backend_file_stats(str8 path);
void *backend_file_open(str8 path, int mode);
int backend_file_flush(void *f);
int backend_file_close(void *f);
int backend_file_read(void *f, void *buf, usize buf_size);
int backend_file_write(void *f, const void *buf, usize buf_size);
int backend_file_tell(void *f);
int backend_file_seek(void *f, int pos, int origin);
int backend_file_remove(str8 path);

// Input
int backend_inp(void);
int backend_key(int key);
u8 *backend_keys(void);
f32 backend_crank(void);
int backend_crank_docked(void);

// Menu
void *backend_menu_item_add(const char *title, void (*callback)(void *arg), void *arg);
void *backend_menu_checkmark_add(const char *title, int val, void (*callback)(void *arg), void *arg);
void *backend_menu_options_add(const char *title, const char **options, int count, void (*callback)(void *arg), void *arg);
int backend_menu_value(void *ptr);
void backend_menu_clr(void);

struct exe_path backend_where(void);

// Debug
void backend_draw_debug_shape(struct debug_shape *shapes, int count);
