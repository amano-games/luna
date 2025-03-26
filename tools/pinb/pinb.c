#include "pinb.h"
#include "collisions.h"
#include "path.h"
#include "str.h"
#include "sys-log.h"

#define CUTE_TILED_IMPLEMENTATION
#include "external/cute_tiled.h"

bool32
pinb_obj_is_polygon(struct cute_tiled_object_t *obj)
{
	if(!obj) { return false; }
	return obj->vert_count > 0 && obj->vert_type == 1;
}

usize
pinb_layer_get_rigid_bodies(struct cute_tiled_layer_t *layer)
{
	cute_tiled_object_t *obj = layer->objects;

	while(obj) {
		if(pinb_obj_is_polygon(obj)) {
			struct col_poly poly = {0};
			f32 x, y;
			for(int i = 0; i < obj->vert_count * 2; i += 2) {
				x = obj->vertices[i];
				y = obj->vertices[i + 1];
			}
		}
	}

	return 0;
}

i32
handle_pinball_table(str8 in_path, str8 out_path, struct alloc scratch)
{
	i32 res = 0;
#if 0

	cute_tiled_map_t *map = cute_tiled_load_map_from_file((char *)in_path.str, NULL);
	assert(map);

	cute_tiled_layer_t *layer = map->layers;
	while(layer) {
		sys_printf("layer: %s", layer->name.ptr);
		str8 layer_type = str8_cstr((char *)layer->type.ptr);
		if(str8_match(str8_lit("objectgroup"), layer_type, 0)) {
			sys_printf("found object group layer %s", layer->name.ptr);
		}
		layer = layer->next;
	}

	cute_tiled_free_map(map);
	str8 out_file_path = make_file_name_with_ext(scratch, out_path, str8_lit(PINBALL_EXT));

	log_info("pinb-gen", "%s -> %s\n", in_path.str, out_file_path.str);
#endif
	return res;
}
