#include "pd-utils.h"
#include "base/str.h"
#include "sys/sys-io.h"
#include "base/types.h"

str8
pdxinfo_extract_value_str(str8 line, str8 key)
{
	str8 res = str8_skip_chop_whitespace(str8_skip(line, key.size));
	return res;
}

i32
pdxinfo_extract_value_i32(str8 line, str8 key)
{
	str8 value = str8_skip_chop_whitespace(str8_skip(line, key.size));
	i32 res    = str8_to_i32(value);
	return res;
}

struct pdxinfo
pdxinfo_parse(struct alloc alloc, struct alloc scratch)
{
	struct pdxinfo res                   = {0};
	str8 path                            = str8_lit("pdxinfo");
	struct sys_full_file_res pdxinfodata = sys_load_full_file(scratch, path);
	str8 data                            = {.str = pdxinfodata.data, .size = pdxinfodata.size};

	struct str8_list list = str8_split_by_string_chars(scratch, data, str8_lit("\n"), 0);
	str8 name_key         = str8_lit("name=");
	str8 author_key       = str8_lit("author=");
	str8 description_key  = str8_lit("description=");
	str8 bundle_id_key    = str8_lit("bundleID=");
	str8 version_key      = str8_lit("version=");
	str8 build_number_key = str8_lit("buildNumber=");
	str8 image_path_key   = str8_lit("imagePath=");
	i32 flags             = 0;

	for(struct str8_node *n = list.first; n != 0; n = n->next) {
		str8 line = n->str;

		if(str8_starts_with(line, name_key, flags)) {
			res.name = str8_cpy_push(alloc, pdxinfo_extract_value_str(line, name_key));
		} else if(str8_starts_with(line, author_key, flags)) {
			res.author = str8_cpy_push(alloc, pdxinfo_extract_value_str(line, author_key));
		} else if(str8_starts_with(line, description_key, flags)) {
			res.description = str8_cpy_push(alloc, pdxinfo_extract_value_str(line, description_key));
		} else if(str8_starts_with(line, bundle_id_key, flags)) {
			res.bundle_id = str8_cpy_push(alloc, pdxinfo_extract_value_str(line, bundle_id_key));
		} else if(str8_starts_with(line, version_key, flags)) {
			res.version = str8_cpy_push(alloc, pdxinfo_extract_value_str(line, version_key));
		} else if(str8_starts_with(line, build_number_key, flags)) {
			res.build_number = pdxinfo_extract_value_i32(line, build_number_key);
		} else if(str8_starts_with(line, image_path_key, flags)) {
			res.image_path = str8_cpy_push(alloc, pdxinfo_extract_value_str(line, image_path_key));
		}
	}

	return res;
}
