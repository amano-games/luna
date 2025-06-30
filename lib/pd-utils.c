#include "pd-utils.h"
#include "dbg.h"
#include "str.h"
#include "sys-io.h"
#include "sys-types.h"

void
pdxinfo_extract_value_str(str8 line, str8 key, char *dest, size_t max_len)
{
	str8 value    = str8_skip(line, key.size);
	str8 dest_str = {.str = (u8 *)dest, .size = max_len};
	str8_cpy(&value, &dest_str);
}

i32
pdxinfo_extract_value_i32(str8 line, str8 key)
{
	str8 value = str8_skip_chop_whitespace(str8_skip(line, key.size));
	i32 res    = str8_to_i32(value);
	return res;
}

void
pdxinfo_parse(struct pdxinfo *info, struct alloc scratch)
{
	str8 path = str8_lit("pdxinfo");
	void *f   = sys_file_open_r(str8_lit("pdxinfo"));
	dbg_check(f, "pd", "failed to open pdxinfo");

	struct sys_full_file_res res = sys_load_full_file(path, scratch);
	sys_file_close(f);

	str8 data = {.str = res.data, .size = res.size};

	struct str8_list list = str8_split_by_string_chars(scratch, data, str8_lit("\n"), 0);
	str8 name_key         = str8_lit("name=");
	str8 author_key       = str8_lit("author=");
	str8 description_key  = str8_lit("description=");
	str8 bundle_id_key    = str8_lit("bundleID=");
	str8 version_key      = str8_lit("version=");
	str8 build_number_key = str8_lit("buildNumber=");
	str8 image_path_key   = str8_lit("imagePath=");

	for(struct str8_node *n = list.first; n != 0; n = n->next) {
		str8 line = n->str;
		i32 flags = 0;

		if(str8_starts_with(line, name_key, flags)) {
			pdxinfo_extract_value_str(line, name_key, info->name, sizeof(info->name));
		} else if(str8_starts_with(line, author_key, flags)) {
			pdxinfo_extract_value_str(line, author_key, info->author, sizeof(info->author));
		} else if(str8_starts_with(line, description_key, flags)) {
			pdxinfo_extract_value_str(line, description_key, info->description, sizeof(info->description));
		} else if(str8_starts_with(line, bundle_id_key, flags)) {
			pdxinfo_extract_value_str(line, bundle_id_key, info->bundle_id, sizeof(info->bundle_id));
		} else if(str8_starts_with(line, version_key, flags)) {
			pdxinfo_extract_value_str(line, version_key, info->version, sizeof(info->version));
		} else if(str8_starts_with(line, build_number_key, flags)) {
			info->build_number = pdxinfo_extract_value_i32(line, build_number_key);
		} else if(str8_starts_with(line, image_path_key, flags)) {
			pdxinfo_extract_value_str(line, image_path_key, info->image_path, sizeof(info->image_path));
		}
	}

	return;

error:
	if(f != NULL) {
		sys_file_close(f);
	}
	info = NULL;
	return;
}
