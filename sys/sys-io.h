#pragma once

#include "sys-backend.h"
#include "sys-types.h"

enum {                  // pd_api.h:
	SYS_FILE_R = 1 | 2, // kFileRead | kFileReadData
	SYS_FILE_W = 4,     // kFileWrite
	SYS_FILE_A = (2 << 2),
};

enum {
	SYS_FILE_SEEK_SET, // SEEK_SET
	SYS_FILE_SEEK_CUR, // SEEK_CUR
	SYS_FILE_SEEK_END, // SEEK_END
};

enum {
	SYS_SEEK_SET = SYS_FILE_SEEK_SET,
	SYS_SEEK_CUR = SYS_FILE_SEEK_CUR,
	SYS_SEEK_END = SYS_FILE_SEEK_END,
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

#define sys_file_open   backend_file_open
#define sys_file_close  backend_file_close
#define sys_file_flush  backend_file_flush
#define sys_file_read   backend_file_read
#define sys_file_write  backend_file_write
#define sys_file_tell   backend_file_tell
#define sys_file_seek   backend_file_seek
#define sys_file_remove backend_file_remove

#define sys_tex_load backend_tex_load

typedef void *sys_file;

struct sys_file_stats sys_fstats(str8 path);
sys_file *sys_fopen(str8 path, const char *mode);
int sys_fclose(sys_file *f);
usize sys_fread(void *buf, usize size, usize count, sys_file *f);
usize sys_fwrite(const void *buf, usize size, usize count, sys_file *f);
int sys_ftell(sys_file *f);
int sys_fseek(sys_file *f, int pos, int origin);
int sys_fflush(sys_file *f);
