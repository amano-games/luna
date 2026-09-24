#pragma once

#include "lib/fnt/fnt-defs.h"
#include "lib/serialize/serialize.h"

i32 fnt_char_size_x_px(struct fnt fnt, i32 a, i32 b, i32 tracking);
v2_i32 fnt_size_px(struct fnt fnt, const str8 str, i32 tracking, i32 leading);
i32 fnt_mono_size_x_px(struct fnt fnt, str8 str, i32 tracking);

void fnt_write(struct fnt fnt, struct ser_writer *w);
i32 fnt_read(struct ser_reader *r, struct fnt *fnt);
struct fnt fnt_load_from_mem(struct alloc alloc, void *data, usize size);
struct fnt fnt_load(str8 path, struct alloc alloc, struct alloc scratch);
struct str8_list fnt_wrap_lines_from_str(struct alloc alloc, struct fnt fnt, str8 str, i32 max_width, i32 tracking, i32 leading);
