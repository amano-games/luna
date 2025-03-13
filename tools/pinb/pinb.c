#include "pinb.h"
#include "path.h"
#include "str.h"

i32
handle_pinball_table(str8 in_path, str8 out_path, struct alloc scratch)
{
	i32 res = 0;

	str8 out_file_path = make_file_name_with_ext(scratch, out_path, str8_lit(PINBALL_EXT));

	log_info("pinb-gen", "%s -> %s\n", in_path.str, out_file_path.str);
	return res;
}
