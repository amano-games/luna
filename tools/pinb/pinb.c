#include "pinb.h"
#include "path.h"
#include "str.h"
#include "sys-log.h"

#define CUTE_TILED_IMPLEMENTATION
#include "external/cute_tiled.h"

usize
pinb_layer_get_rigid_bodies(struct cute_tiled_layer_t *layer)
{
	return 0;
}

i32
handle_pinball_table(str8 in_path, str8 out_path, struct alloc scratch)
{
	i32 res = 0;

	cute_tiled_map_t *map = cute_tiled_load_map_from_file((char *)in_path.str, NULL);
	assert(map);

	cute_tiled_layer_t *layer = map->layers;
	while(layer) {
		sys_printf("layer: %s", layer->name.ptr);
		str8 layer_type = str8_cstr((char *)layer->type.ptr);
		if(str8_match(str8_lit("objectgroup"), layer_type, 0)) {
			sys_printf("found object group layer", layer->name.ptr);
		}
		layer = layer->next;
	}

	cute_tiled_free_map(map);
	str8 out_file_path = make_file_name_with_ext(scratch, out_path, str8_lit(PINBALL_EXT));

	log_info("pinb-gen", "%s -> %s\n", in_path.str, out_file_path.str);
	return res;
}
