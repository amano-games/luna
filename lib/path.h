#pragma once

#include "mem.h"
#include "sys-types.h"

enum path_style {
	path_style_none,
	path_style_relative,
	path_style_absolute_windows,
	path_style_absolute_unix,
	// TODO: absolute system should be defined based on platform
	path_style_absolute_system = path_style_absolute_unix,
};

struct str8_list path_split(struct alloc alloc, str8 str);
str8 path_join_by_style(struct alloc alloc, struct str8_list *path, enum path_style style);
void path_resolve_dots_in_place(struct str8_list *path, enum path_style style, struct alloc scratch);
str8 path_resolve_dots(struct alloc alloc, str8 path, enum path_style style, struct alloc scratch);

str8 make_file_name_with_ext(struct alloc alloc, str8 file_name, str8 ext);
enum path_style path_style_from_str8(str8 string);
struct str8_list path_normalized_list_from_string(struct alloc alloc, str8 path_string, enum path_style *style_out, struct alloc scratch);

str8 path_absolute_dst_from_relative_dst_src(struct alloc alloc, str8 dst, str8 src, struct alloc scratch);
str8 path_relative_dst_from_absolute_dst_src(struct alloc alloc, str8 dst, str8 src, struct alloc scratch);
