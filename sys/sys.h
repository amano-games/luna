#pragma once

#include "sys-types.h"

#define SYS_UPS       50
#define SYS_DISPLAY_W 400
#define SYS_DISPLAY_H 240
#if !defined(TARGET_PLAYDATE)
#define SYS_MAX_MEM MMEGABYTE(100)
#else
#define SYS_MAX_MEM 8388208
#endif

#define SYS_DISPLAY_WBYTES 52
#define SYS_DISPLAY_WWORDS 13

struct sys_display {
	u32 *px;
	int w;
	int h;
	int wword;
	int wbyte;
};

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

struct app_metadata {
	char name[50];
	char author[50];
	char description[100];
	char bundle_id[100];
	char version[100];
	i32 build_number;
	char image_path[100];
};

void app_init(struct app_mem mem);
void app_tick(f32 dt);
void app_draw(void);
void app_close(void);
void app_resume(void);
void app_pause(void);

struct sys_display sys_display(void);
f32 sys_seconds(void);
void sys_display_update_rows(int a, int b);

void sys_menu_item_add(int id, const char *title, void (*callback)(void *arg), void *arg);
void sys_menu_checkmark_add(int id, const char *title, int val, void (*callback)(void *arg), void *arg);
int sys_menu_value(int id);
void sys_menu_options_add(int id, const char *title, const char **options, int count, void (*callback)(void *arg), void *arg);
void sys_menu_clr(void);
