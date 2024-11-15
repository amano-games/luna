#pragma once

#include "mem.h"
#include "sys-types.h"

str8 make_file_name_with_ext(struct alloc alloc, str8 file_name, str8 ext);
str8 make_file_path_with_ext(struct alloc alloc, str8 file_name, str8 ext);
