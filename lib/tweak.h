#pragma once

#define DEV
#define DEBUG true

// https://web.archive.org/web/20130624102139/https://mollyrocket.com/forums/viewtopic.php?p=3355
// https://blog.voxagon.se/2018/03/13/hot-reloading-hardcoded-parameters.html

#if defined(DEV) && DEBUG
#define TWEAK_I32(constant) tweak_value_i32(__FILE__, __LINE__, __COUNTER__, constant)
#define TWEAK_F32(constant) tweak_value_f32(__FILE__, __LINE__, __COUNTER__, constant)
#else
#define TWEAK_I32(constant) constant
#define TWEAK_F32(constant) constant
#endif

#if defined(DEV) && DEBUG && 0
i32 tweak_look_up_value_i32(char *src_file, i32 line_number, i32 counter, i32 constant);
f32 tweak_look_up_value_f32(char *src_file, i32 line_number, i32 counter, f32 constant);

void tweak_reload_values_for_file(char *src_file);
void tweak_reload_all(void);
void tweak_reload_changed_hot_values(void);

inline i32
tweak_value_i32(char *src_path, i32 line_number, i32 counter, i32 constant)
{
	return tweak_look_up_value_i32(src_path, line_number, counter, constant);
}
inline f32
tweak_value_f32(char *src_path, i32 line_number, i32 counter, f32 constant)
{
	return tweak_look_up_value_f32(src_path, line_number, counter, constant);
}

// TODO: implement the actual reload of the value
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
