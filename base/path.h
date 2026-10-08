#pragma once

#include "base/context-cracking.h"
#include "base/mem.h"
#include "base/types.h"

enum path_style {
	path_style_none,
	path_style_relative,
	path_style_absolute_windows,
	path_style_absolute_unix,

#if OS_WINDOWS
	path_style_absolute_system = path_style_absolute_windows,
#elif OS_LINUX || OS_MACOS || OS_WASM || OS_PLAYDATE
	path_style_absolute_system = path_style_absolute_unix,
#else
#error Absolute path style is undefined for this OS.
#endif
};

struct str8_list path_split(struct alloc alloc, str8 str);
str8 path_join_by_style(struct alloc alloc, struct str8_list *path, enum path_style style);
void path_resolve_dots_in_place(struct str8_list *path, enum path_style style, struct alloc scratch);
str8 path_resolve_dots(struct alloc alloc, str8 path, enum path_style style, struct alloc scratch);

str8 path_make_file_name_with_ext(struct alloc alloc, str8 file_name, str8 ext);
enum path_style path_style_from_str8(str8 string);
struct str8_list path_normalized_list_from_string(struct alloc alloc, str8 path_string, enum path_style *style_out, struct alloc scratch);

str8 path_absolute_dst_from_relative_dst_src(struct alloc alloc, str8 dst, str8 src, struct alloc scratch);
str8 path_relative_dst_from_absolute_dst_src(struct alloc alloc, str8 dst, str8 src, struct alloc scratch);
