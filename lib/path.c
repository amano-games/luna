#include "path.h"
#include "str.h"

str8
make_file_name_with_ext(struct alloc alloc, str8 file_name, str8 ext)
{
	str8 file_name_no_ext = str8_chop_last_dot(file_name);
	str8 result           = str8_fmt_push(alloc, "%.*s.%.*s", (i32)file_name_no_ext.size, file_name_no_ext.str, (i32)ext.size, ext.str);
	return result;
}
