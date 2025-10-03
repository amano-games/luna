#pragma once

// https://web.archive.org/web/20130624102139/https://mollyrocket.com/forums/viewtopic.php?p=3355
// https://blog.voxagon.se/2018/03/13/hot-reloading-hardcoded-parameters.html

// #if defined(DEV) && DEBUG
// #define TWEAK_VALUES true
// #else
// #define TWEAK_VALUES false
// #endif
// #define TWEAK_VALUES true

#if defined(TWEAK_VALUES)
#include "base/types.h"
#include "base/marena.h"
#include "base/str.h"
#include "base/log.h"
#include "base/dbg.h"
#include "sys/sys-io.h"
#include "sys/sys.h"
#endif

#if defined(TWEAK_VALUES)
#define TWEAK_I32(constant) tweak_value_i32(__FILE__, __LINE__, __COUNTER__, constant)
#define TWEAK_F32(constant) tweak_value_f32(__FILE__, __LINE__, __COUNTER__, constant)
#else
#define TWEAK_I32(constant) constant
#define TWEAK_F32(constant) constant
#endif

#if defined(TWEAK_VALUES)
i32 tweak_look_up_value_i32(char *src_file, i32 line_number, i32 counter, i32 constant);
f32 tweak_look_up_value_f32(char *src_file, i32 line_number, i32 counter, f32 constant);

void tweak_reload_values_for_file(char *src_file);
void tweak_reload_all(void);
void tweak_reload_changed_hot_values(void);

i32
tweak_value_i32(char *src_path, i32 line_number, i32 counter, i32 constant)
{
	return tweak_look_up_value_i32(src_path, line_number, counter, constant);
}
f32
tweak_value_f32(char *src_path, i32 line_number, i32 counter, f32 constant)
{
	return tweak_look_up_value_f32(src_path, line_number, counter, constant);
}

// TODO: implement the actual reload of the value
f32
tweak_look_up_value_f32(
	char *src_file,
	i32 line_number,
	i32 counter,
	f32 constant)
{
	usize mem_size = MKILOBYTE(10);
	void *mem      = sys_alloc(NULL, mem_size);
	void *f        = NULL;
	dbg_check_warn(mem, "tweak", "failed to get memory");
	struct marena marena = {0};
	marena_init(&marena, mem, mem_size);
	struct alloc alloc = marena_allocator(&marena);
	str8 exe_path      = sys_exe_path();
	str8 project_root  = str8_chop_last_slash(str8_chop_last_slash(str8_chop_last_slash(exe_path)));
	str8 full_path     = str8_fmt_push(alloc, "%.*s/%s", project_root.size, project_root.str, src_file);
	f                  = sys_file_open_r(full_path);
	dbg_check_warn(f, "tweak", "failed to open the file %s", full_path.str);

	struct sys_full_file_res res = sys_load_full_file(full_path, alloc);
	u8 *buffer                   = res.data;
	size buffer_size             = res.size;
	str8 str_line                = {.str = buffer, .size = buffer_size};
	usize new_line_pos           = 0;
	i32 current_line_index       = 0;
	str8 line                    = {0};

	while(buffer_size > 0) {
		new_line_pos = str8_find_needle(str_line, 0, str8_lit("\n"), 0);
		if(new_line_pos == str_line.size) {
			// newline not found, need more data
			break;
		}

		// process the line
		line = (str8){.str = (u8 *)buffer, .size = new_line_pos};
		if(current_line_index == line_number) {
			break;
		}
		current_line_index++;

		// remove processed line + newline
		// TODO: instead of moving the memory we can just move the string position or something
		// Find needle function can start from a position
		mmov(buffer, buffer + new_line_pos + 1, buffer_size - (new_line_pos + 1));
		buffer_size -= (new_line_pos + 1);

		// update str_line
		str_line.str  = (u8 *)buffer;
		str_line.size = buffer_size;
	}
	sys_printf("tweak value: %.*s\n", (int)line.size, line.str);

error:;
	if(f != NULL) {
		sys_file_close(f);
	}
	if(mem != NULL) {
		sys_free(mem);
	}
	return constant;
}

i32
tweak_look_up_value_i32(
	char *src_file,
	i32 line_number,
	i32 counter,
	i32 constant)
{
	return constant;
}

#else
inline void
tweak_reload_values_for_file(char *src_path)
{
}

inline void
tweak_reload_all(void)
{
}

inline void
tweak_reload_changed_hot_values(void)
{
}
#endif
