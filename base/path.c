#include "path.h"
#include "base/link-list.h"
#include "base/str.h"
#include "sys/sys-io.h"
#include "base/types.h"
#include "base/dbg.h"

// TODO: accept OS enum
str_match_flags
path_match_flags_from_os(void)
{
	str_match_flags flags = str_match_flag_slash_insensitive;
	// switch(os) {
	// default: {
	// } break;
	// case OperatingSystem_Windows: {
	// 	flags |= str_match_flag_case_insensitive;
	// } break;
	// case OperatingSystem_Linux:
	// case OperatingSystem_Mac: {
	// } break;
	// }
	return flags;
}

struct str8_list
path_split(struct alloc alloc, str8 str)
{
	struct str8_list res = str8_split(alloc, str, (u8 *)"/\\", 2, 0);
	return res;
}

str8
path_join_by_style(struct alloc alloc, struct str8_list *path, enum path_style style)
{
	struct str_join params = {0};
	switch(style) {
	case path_style_none: {
	} break;
	case path_style_relative:
	case path_style_absolute_windows: {
		params.sep = str8_lit("/");
	} break;

	case path_style_absolute_unix: {
		params.pre = str8_lit("/");
		params.sep = str8_lit("/");
	} break;
	}

	str8 result = str8_list_join(alloc, path, &params);
	return result;
}

str8
path_make_file_name_with_ext(struct alloc alloc, str8 file_name, str8 ext)
{
	str8 file_name_no_ext = str8_chop_last_dot(file_name);
	str8 result           = str8_fmt_push(alloc, "%.*s.%.*s", (i32)file_name_no_ext.size, file_name_no_ext.str, (i32)ext.size, ext.str);
	return result;
}

enum path_style
path_style_from_str8(str8 str)
{
	enum path_style res = path_style_relative;
	if(str.size >= 1 && str.str[0] == '/') {
		res = path_style_absolute_unix;
	} else if(str.size >= 2 &&
		char_is_alpha(str.str[0]) &&
		str.str[1] == ':') {
		if(str.size == 2 ||
			char_is_slash(str.str[2])) {
			res = path_style_absolute_windows;
		}
	}
	return (res);
}

struct str8_list
path_normalized_list_from_string(
	struct alloc alloc,
	str8 path_string,
	enum path_style *style_out,
	struct alloc scratch)
{
	// analyze path
	enum path_style path_style = path_style_from_str8(path_string);
	struct str8_list path      = path_split(alloc, path_string);

	// prepend current path to convert relative -> absolute
	enum path_style path_style_full = path_style;
	if(path.node_count != 0 && path_style == path_style_relative) {
		str8 where = sys_exe_path();

		enum path_style current_path_style = path_style_from_str8(where);
		dbg_assert(current_path_style != path_style_relative);
		struct str8_list current_path = path_split(alloc, where);
		str8_list_concat_in_place(&current_path, &path);
		path            = current_path;
		path_style_full = current_path_style;
	}

	// resolve dots
	path_resolve_dots_in_place(&path, path_style_full, scratch);

	// return
	if(style_out != 0) {
		*style_out = path_style_full;
	}
	return (path);
}

str8
path_normalized_from_string(struct alloc alloc, str8 path_string, struct alloc scratch)
{
	enum path_style style = path_style_relative;
	struct str8_list path = path_normalized_list_from_string(scratch, path_string, &style, scratch);

	str8 result = path_join_by_style(alloc, &path, style);
	return (result);
}

str8
path_absolute_dst_from_relative_dst_src(
	struct alloc alloc,
	str8 dst,
	str8 src,
	struct alloc scratch)
{
	str8 result               = dst;
	enum path_style dst_style = path_style_from_str8(dst);
	if(dst_style == path_style_relative) {
		str8 dst_from_src_absolute            = str8_fmt_push(alloc, "%.*s/%.*s", (i32)src.size, src.str, (i32)dst.size, dst.str);
		str8 dst_from_src_absolute_normalized = path_normalized_from_string(alloc, dst_from_src_absolute, scratch);
		result                                = dst_from_src_absolute_normalized;
	}
	return result;
}

str8
path_relative_dst_from_absolute_dst_src(struct alloc alloc, str8 dst, str8 src, struct alloc scratch)
{
	// rjf: gather path parts
	str8 dst_name                = str8_skip_last_slash(dst);
	str8 src_folder              = str8_chop_last_slash(src);
	str8 dst_folder              = str8_chop_last_slash(dst);
	struct str8_list src_folders = path_split(scratch, src_folder);
	struct str8_list dst_folders = path_split(scratch, dst_folder);

	// rjf: count # of backtracks to get from src -> dest
	usize num_backtracks = src_folders.node_count;
	for(struct str8_node *src_n = src_folders.first, *bp_n = dst_folders.first;
		src_n != 0 && bp_n != 0;
		src_n = src_n->next, bp_n = bp_n->next) {
		if(str8_match(src_n->str, bp_n->str, path_match_flags_from_os())) {
			num_backtracks -= 1;
		} else {
			break;
		}
	}

	// rjf: only build relative string if # of backtracks is not the entire `src`.
	// if getting to `dst` from `src` requires erasing the entire `src`, then the
	// only possible way to get to `dst` from `src` is via absolute path.
	str8 res = {0};
	if(num_backtracks >= src_folders.node_count) {
		res = path_normalized_from_string(alloc, dst, scratch);
	} else {
		// rjf: build backtrack parts
		struct str8_list dst_path_strs = {0};
		for(usize idx = 0; idx < num_backtracks; idx += 1) {
			str8_list_push(scratch, &dst_path_strs, str8_lit(".."));
		}

		// rjf: build parts of dst which are unique from src
		{
			b32 unique_from_src = false;
			for(struct str8_node *src_n = src_folders.first, *bp_n = dst_folders.first;
				bp_n != 0;
				bp_n = bp_n->next) {
				if(!unique_from_src && (src_n == 0 || !str8_match(src_n->str, bp_n->str, path_match_flags_from_os()))) {

					unique_from_src = 1;
				}
				if(unique_from_src) {
					str8_list_push(scratch, &dst_path_strs, bp_n->str);
				}
				if(src_n != 0) {
					src_n = src_n->next;
				}
			}
		}

		// rjf: build file name
		str8_list_push(scratch, &dst_path_strs, dst_name);

		// rjf: join
		struct str_join join = {0};
		{
			join.sep = str8_lit("/");
		}
		res = str8_list_join(alloc, &dst_path_strs, &join);
	}

	return res;
}

str8
path_resolve_dots(
	struct alloc alloc,
	str8 path,
	enum path_style style,
	struct alloc scratch)
{
	str8 res                   = {0};
	struct str8_list path_list = path_split(scratch, path);
	path_resolve_dots_in_place(&path_list, style, scratch);
	res = path_join_by_style(alloc, &path_list, style);
	return res;
}

void
path_resolve_dots_in_place(
	struct str8_list *path,
	enum path_style style,
	struct alloc scratch)
{

	struct str8_meta_node *stack          = 0;
	struct str8_meta_node *free_meta_node = 0;
	struct str8_node *first               = path->first;

	mclr_struct(path);
	for(struct str8_node *node = first, *next = 0;
		node != 0;
		node = next) {
		// save next now
		next = node->next;

		// cases:
		if(node == first && style == path_style_absolute_windows) {
			goto save_without_stack;
		}
		if(node->str.size == 1 && node->str.str[0] == '.') {
			goto do_nothing;
		}
		if(node->str.size == 2 && node->str.str[0] == '.' && node->str.str[1] == '.') {
			if(stack != 0) {
				goto eliminate_stack_top;
			} else {
				goto save_without_stack;
			}
		}
		goto save_with_stack;

	// handlers:
	save_with_stack: {
		str8_list_push_node(path, node);

		struct str8_meta_node *stack_node = free_meta_node;
		if(stack_node != 0) {
			SLLStackPop(free_meta_node);
		} else {
			stack_node = scratch.allocf(scratch.ctx, sizeof(struct str8_meta_node));
		}
		SLLStackPush(stack, stack_node);
		stack_node->node = node;

		continue;
	}

	save_without_stack: {
		str8_list_push_node(path, node);

		continue;
	}

	eliminate_stack_top: {
		path->node_count -= 1;
		path->total_size -= stack->node->str.size;

		SLLStackPop(stack);

		if(stack == 0) {
			path->last = path->first;
		} else {
			path->last = stack->node;
		}
		continue;
	}

	do_nothing:
		continue;
	}
}
