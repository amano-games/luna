#pragma once

#include "lib/mem.h"
#include "sys-types.h"

enum {
	SYS_FILE_MODE_R,
	SYS_FILE_MODE_W,
	SYS_FILE_MODE_A,
};

enum {
	SYS_FILE_SEEK_SET, // SEEK_SET
	SYS_FILE_SEEK_CUR, // SEEK_CUR
	SYS_FILE_SEEK_END, // SEEK_END
};

struct sys_file_stats {
	i32 isdir;
	u32 size;
	i32 m_year;
	i32 m_month;
	i32 m_day;
	i32 m_hour;
	i32 m_minute;
	i32 m_second;
};

typedef void *sys_file;

struct sys_full_file_res {
	void *data;
	usize size;
};

void *sys_file_open(str8 path, i32 sys_file_mode);
void *sys_file_open_r(str8 path);
void *sys_file_open_w(str8 path);
void *sys_file_open_a(str8 path);
b32 sys_file_close(void *f);
b32 sys_file_del(str8 path);
b32 sys_file_rename(str8 from, str8 to);
b32 sys_file_flush(void *f);
i32 sys_file_tell(void *f);
i32 sys_file_seek_set(void *f, i32 pos);
i32 sys_file_seek_cur(void *f, i32 pos);
i32 sys_file_seek_end(void *f, i32 pos);
i32 sys_file_w(void *f, const void *buf, u32 bsize);
i32 sys_file_r(void *f, void *buf, u32 bsize);
struct sys_file_stats sys_file_stats(str8 path);

struct sys_full_file_res sys_load_full_file(str8 path, struct alloc alloc);
usize sys_file_modified(str8 path);
str8 sys_exe_path(void);
str8 sys_base_path(void);
str8 sys_pref_path(void);
