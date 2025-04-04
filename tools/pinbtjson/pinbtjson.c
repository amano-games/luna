#include "pinbtjson.h"
#include "arr.h"
#include "json.h"
#include "mathfunc.h"
#include "mem-arena.h"
#include "path.h"
#include "pinb/pinb-ser.h"
#include "serialize/serialize.h"
#include "sys-assert.h"
#include "sys.h"
#include "tools/tex/tex.h"

struct pinb_col_cir_res
pinbjson_handle_col_cir(str8 json, jsmntok_t *tokens, i32 index)
{
	struct pinb_col_cir_res res = {0};
	jsmntok_t *root             = &tokens[index];
	assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);

	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		if(json_eq(json, key, str8_lit("x")) == 0) {
			res.cir.p.x = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("y")) == 0) {
			res.cir.p.y = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("r")) == 0) {
			res.cir.r = json_parse_f32(json, value);
		}
	}

	return res;
}

struct pinb_col_shape_res
pinbjson_handle_col_shape(str8 json, jsmntok_t *tokens, i32 index)
{
	struct pinb_col_shape_res res = {0};
	jsmntok_t *root               = &tokens[index];
	assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);
	for(usize i = index + 1; i < index + res.token_count; i++) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		if(json_eq(json, key, str8_lit("poly")) == 0) {
			assert(value->type == JSMN_ARRAY);
			i++;
			res.shape_count          = 1;
			res.shapes[0].type       = COL_TYPE_POLY;
			res.shapes[0].poly.count = 1;
			usize vert_count         = value->size / 2;
			assert(vert_count < ARRLEN(res.shapes[0].poly.sub_polys[0].verts));
			for(usize j = 0; j < vert_count; ++j) {
				res.shapes[0].poly.sub_polys[0].count++;
				res.shapes[0].poly.sub_polys[0].verts[j].x = json_parse_f32(json, tokens + ++i);
				res.shapes[0].poly.sub_polys[0].verts[j].y = json_parse_f32(json, tokens + ++i);
			}
		} else if(json_eq(json, key, str8_lit("aabb")) == 0) {
			assert(value->type == JSMN_ARRAY);
			assert(value->size == 4);
			i++;
			res.shape_count          = 1;
			res.shapes[0].type       = COL_TYPE_AABB;
			res.shapes[0].aabb.min.x = json_parse_f32(json, tokens + ++i);
			res.shapes[0].aabb.min.y = json_parse_f32(json, tokens + ++i);
			res.shapes[0].aabb.max.x = json_parse_f32(json, tokens + ++i);
			res.shapes[0].aabb.max.y = json_parse_f32(json, tokens + ++i);
		} else if(json_eq(json, key, str8_lit("cir")) == 0) {
			i++;
			struct pinb_col_cir_res item_res = pinbjson_handle_col_cir(json, tokens, i);
			res.shapes[0].type               = COL_TYPE_CIR;
			res.shapes[0].cir                = item_res.cir;
			i += item_res.token_count;
		} else if(json_eq(json, key, str8_lit("capsule")) == 0) {
			assert(value->type == JSMN_ARRAY);
			i += 2;
			res.shape_count    = 1;
			res.shapes[0].type = COL_TYPE_CAPSULE;
			for(i32 j = 0; j < value->size; j++) {
				jsmntok_t *item                  = tokens + i;
				struct pinb_col_cir_res item_res = pinbjson_handle_col_cir(json, tokens, i);
				res.shapes[0].capsule.cirs[j]    = item_res.cir;
				i += item_res.token_count;
			}
		}
	}

	return res;
}

struct pinb_rigid_body_res
pinbjson_handle_rigid_body(str8 json, jsmntok_t *tokens, i32 index)
{
	struct pinb_rigid_body_res res = {0};
	jsmntok_t *root                = &tokens[index];
	assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);
	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];
		if(json_eq(json, key, str8_lit("flags")) == 0) {
			res.body.flags = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("angular_damping")) == 0) {
			res.body.ang_damping = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("linear_damping")) == 0) {
			res.body.linear_damping = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("dynamic_friction")) == 0) {
			res.body.dynamic_friction = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("mass")) == 0) {
			res.body.mass = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("restitution")) == 0) {
			res.body.restitution = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("static_friction")) == 0) {
			res.body.static_friction = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("collision_shape")) == 0) {
			++i;
			struct pinb_col_shape_res item_res = pinbjson_handle_col_shape(json, tokens, i);
			res.body.shape                     = item_res.shapes[0];
			i += item_res.token_count;
		}
	}
	return res;
}

struct pinb_entity_res
pinbjson_handle_entity(str8 json, jsmntok_t *tokens, i32 index, struct alloc alloc)
{
	struct pinb_entity_res res = {0};
	jsmntok_t *root            = &tokens[index];
	assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);

	for(usize i = index + 1; i < index + res.token_count; i++) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];
		if(json.str[key->start] == '_') {
			++i;
		} else if(json_eq(json, key, str8_lit("id")) == 0) {
			res.entity.id = json_parse_i32(json, value);
			++i;
		} else if(json_eq(json, key, str8_lit("x")) == 0) {
			res.entity.x = json_parse_i32(json, value);
			++i;
		} else if(json_eq(json, key, str8_lit("y")) == 0) {
			res.entity.y = json_parse_i32(json, value);
			++i;
		} else if(json_eq(json, key, str8_lit("spr")) == 0) {
			assert(value->type == JSMN_OBJECT);
			++i;
			jsmntok_t *path_key       = &tokens[++i];
			jsmntok_t *path_value     = &tokens[++i];
			jsmntok_t *flip_key       = &tokens[++i];
			jsmntok_t *flip_value     = &tokens[++i];
			jsmntok_t *offset_key     = &tokens[++i];
			jsmntok_t *offset_value   = &tokens[++i];
			jsmntok_t *offset_x_value = &tokens[++i];
			jsmntok_t *offset_y_value = &tokens[++i];

			assert(path_key->type == JSMN_STRING);
			assert(path_value->type == JSMN_STRING);
			assert(flip_key->type == JSMN_STRING);
			assert(flip_value->type == JSMN_PRIMITIVE);
			assert(offset_key->type == JSMN_STRING);
			assert(offset_value->type == JSMN_ARRAY);
			assert(offset_x_value->type == JSMN_PRIMITIVE);
			assert(offset_y_value->type == JSMN_PRIMITIVE);
			assert(json_eq(json, path_key, str8_lit("path")) == 0);
			assert(json_eq(json, offset_key, str8_lit("offset")) == 0);

			str8 path               = json_str8(json, path_value);
			res.entity.spr.path     = make_file_name_with_ext(alloc, path, str8_lit(TEX_EXT));
			res.entity.spr.flip     = json_parse_i32(json, flip_value);
			res.entity.spr.offset.x = json_parse_f32(json, offset_x_value);
			res.entity.spr.offset.y = json_parse_f32(json, offset_y_value);

		} else if(json_eq(json, key, str8_lit("rigid_body")) == 0) {
			struct pinb_rigid_body_res item_res = pinbjson_handle_rigid_body(json, tokens, i + 1);
			res.entity.body                     = item_res.body;
			i += item_res.token_count;
		} else if(json_eq(json, key, str8_lit("reactive_impulse")) == 0) {
			assert(value->type == JSMN_OBJECT);
			++i;
			jsmntok_t *mag_key    = &tokens[++i];
			jsmntok_t *mag_value  = &tokens[++i];
			jsmntok_t *norm_key   = &tokens[++i];
			jsmntok_t *norm_value = &tokens[++i];

			assert(mag_key->type == JSMN_STRING);
			assert(json_eq(json, mag_key, str8_lit("magnitude")) == 0);
			assert(mag_value->type == JSMN_PRIMITIVE);
			assert(norm_key->type == JSMN_STRING);
			assert(json_eq(json, norm_key, str8_lit("normalize")) == 0);
			assert(norm_value->type == JSMN_PRIMITIVE);

			res.entity.reactive_impulse.magnitude = json_parse_f32(json, mag_value);
			res.entity.reactive_impulse.normalize = json_parse_bool32(json, norm_value);

		} else if(json_eq(json, key, str8_lit("plunger")) == 0) {
			assert(value->type == JSMN_OBJECT);
			++i;
			jsmntok_t *charge_force_max_key    = &tokens[++i];
			jsmntok_t *charge_force_max_value  = &tokens[++i];
			jsmntok_t *charge_force_min_key    = &tokens[++i];
			jsmntok_t *charge_force_min_value  = &tokens[++i];
			jsmntok_t *release_force_max_key   = &tokens[++i];
			jsmntok_t *release_force_max_value = &tokens[++i];
			jsmntok_t *release_force_min_key   = &tokens[++i];
			jsmntok_t *release_force_min_value = &tokens[++i];

			assert(charge_force_max_key->type == JSMN_STRING);
			assert(json_eq(json, charge_force_max_key, str8_lit("charge_force_max")) == 0);
			assert(charge_force_max_value->type == JSMN_PRIMITIVE);
			assert(charge_force_min_key->type == JSMN_STRING);
			assert(json_eq(json, charge_force_min_key, str8_lit("charge_force_min")) == 0);
			assert(charge_force_min_value->type == JSMN_PRIMITIVE);
			assert(release_force_max_key->type == JSMN_STRING);
			assert(json_eq(json, release_force_max_key, str8_lit("release_force_max")) == 0);
			assert(release_force_max_value->type == JSMN_PRIMITIVE);
			assert(release_force_min_key->type == JSMN_STRING);
			assert(json_eq(json, release_force_min_key, str8_lit("release_force_min")) == 0);
			assert(release_force_min_value->type == JSMN_PRIMITIVE);

			res.entity.plunger.charge_force_max  = json_parse_f32(json, charge_force_max_value);
			res.entity.plunger.charge_force_min  = json_parse_f32(json, charge_force_min_value);
			res.entity.plunger.release_force_max = json_parse_f32(json, release_force_max_value);
			res.entity.plunger.release_force_min = json_parse_f32(json, release_force_min_value);
		} else if(json_eq(json, key, str8_lit("flipper")) == 0) {
			assert(value->type == JSMN_OBJECT);
			++i;
			jsmntok_t *flip_type_key   = &tokens[++i];
			jsmntok_t *flip_type_value = &tokens[++i];

			assert(flip_type_key->type == JSMN_STRING);
			assert(json_eq(json, flip_type_key, str8_lit("flip_type")) == 0);
			assert(flip_type_value->type == JSMN_PRIMITIVE);

			res.entity.flipper.flip_type = json_parse_i32(json, flip_type_value);
		}
	}

	return res;
}

struct pinb_physics_props_res
pinbjson_handle_physics_props(str8 json, jsmntok_t *tokens, i32 index)
{
	struct pinb_physics_props_res res = {0};
	jsmntok_t *root                   = &tokens[index];
	assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);
	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		str8 key_str     = json_str8(json, key);
		str8 value_str   = json_str8(json, value);
		if(json_eq(json, key, str8_lit("steps")) == 0) {
			res.props.steps = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("max_translation")) == 0) {
			res.props.max_translation = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("max_rotation")) == 0) {
			res.props.max_rotation = json_parse_f32(json, value) * PI_FLOAT;
		} else if(json_eq(json, key, str8_lit("penetration_correction")) == 0) {
			res.props.penetration_correction = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("penetration_allowance")) == 0) {
			res.props.penetration_allowance = json_parse_f32(json, value);
		}
	}
	return res;
}

struct pinb_flippers_props_res
pinbjson_handle_flippers_props(str8 json, jsmntok_t *tokens, i32 index)
{
	struct pinb_flippers_props_res res = {0};
	jsmntok_t *root                    = &tokens[index];
	assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);
	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		str8 key_str     = json_str8(json, key);
		str8 value_str   = json_str8(json, value);
		if(json_eq(json, key, str8_lit("flip_velocity")) == 0) {
			res.props.flip_velocity = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("release_velocity")) == 0) {
			res.props.release_velocity = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("rotation_max_degrees")) == 0) {
			res.props.rotation_max_turns = json_parse_f32(json, value) * DEG_TO_TURN;
		} else if(json_eq(json, key, str8_lit("rotation_min_degrees")) == 0) {
			res.props.rotation_min_turns = json_parse_f32(json, value) * DEG_TO_TURN;
		} else if(json_eq(json, key, str8_lit("velocity_easing_function")) == 0) {
			res.props.velocity_easing_function = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("velocity_radius_max")) == 0) {
			res.props.velocity_radius_max = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("velocity_radius_min")) == 0) {
			res.props.velocity_radius_min = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("velocity_scale")) == 0) {
			res.props.velocity_scale = json_parse_f32(json, value);
		}
	}
	return res;
}

struct pinb_table_props_res
pinbjson_handle_table_props(str8 json, jsmntok_t *tokens, i32 index)
{
	struct pinb_table_props_res res = {0};
	jsmntok_t *root                 = &tokens[index];
	assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);
	for(usize i = index + 1; i < index + res.token_count; i++) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		str8 key_str     = json_str8(json, key);
		str8 value_str   = json_str8(json, value);
		if(json_eq(json, key, str8_lit("physics_props")) == 0) {
			struct pinb_physics_props_res item_res = pinbjson_handle_physics_props(json, tokens, i + 1);
			res.props.physics_props                = item_res.props;
			i += item_res.token_count;
		} else if(json_eq(json, key, str8_lit("flippers_props")) == 0) {
			struct pinb_flippers_props_res item_res = pinbjson_handle_flippers_props(json, tokens, i + 1);
			res.props.flippers_props                = item_res.props;
			i += item_res.token_count;
		}
	}
	return res;
}

struct pinb_table
pinbjson_handle_pinbjson(str8 json, struct alloc alloc, struct alloc scratch)
{
	struct pinb_table res = {0};

	jsmn_parser parser;
	jsmn_init(&parser);
	i32 token_count = jsmn_parse(&parser, (char *)json.str, json.size, NULL, 0);
	jsmn_init(&parser);
	jsmntok_t *tokens = arr_ini(token_count, sizeof(jsmntok_t), scratch);
	i32 json_res      = jsmn_parse(&parser, (char *)json.str, json.size, tokens, token_count);
	assert(json_res == token_count);

	for(usize i = 1; i < (usize)token_count; i++) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];
		str8 key_str     = json_str8(json, key);
		str8 value_str   = json_str8(json, value);
		if(json_eq(json, key, str8_lit("version")) == 0) {
			assert(value->type == JSMN_PRIMITIVE);
			res.version = json_parse_i32(json, value);
			++i;
		} else if(json_eq(json, key, str8_lit("props")) == 0) {
			assert(value->type == JSMN_OBJECT);
			struct pinb_table_props_res item_res = pinbjson_handle_table_props(json, tokens, i + 1);
			res.props                            = item_res.props;
			i += item_res.token_count;
		} else if(json_eq(json, key, str8_lit("entities_count")) == 0) {
			assert(value->type == JSMN_PRIMITIVE);
			res.entities_count = json_parse_i32(json, value);
			res.entities       = arr_ini(res.entities_count, sizeof(*res.entities), alloc);
			++i;
		} else if(json_eq(json, key, str8_lit("entities")) == 0) {
			assert(value->type == JSMN_ARRAY);
			for(i32 j = 0; j < value->size; j++) {
				i32 item_index  = i + 2;
				jsmntok_t *item = tokens + item_index;
				assert(item->type == JSMN_OBJECT);
				struct pinb_entity_res item_res = pinbjson_handle_entity(json, tokens, item_index, alloc);
				arr_push(res.entities, item_res.entity);
				i += item_res.token_count;
			}
			++i;
		}
	}
	return res;
}

i32
pinbtjson_handle(str8 in_path, str8 out_path)
{
	usize mem_size = MMEGABYTE(1);
	u8 *mem_buffer = sys_alloc(NULL, mem_size);
	assert(mem_buffer != NULL);
	struct marena marena = {0};
	marena_init(&marena, mem_buffer, mem_size);
	struct alloc alloc = marena_allocator(&marena);

	usize scratch_mem_size = MMEGABYTE(10);
	u8 *scratch_mem_buffer = sys_alloc(NULL, scratch_mem_size);
	assert(scratch_mem_buffer != NULL);
	struct marena scratch_marena = {0};
	marena_init(&scratch_marena, scratch_mem_buffer, scratch_mem_size);
	struct alloc scratch = marena_allocator(&scratch_marena);

	str8 json = {0};
	json_load(in_path, alloc, &json);
	struct pinb_table table = pinbjson_handle_pinbjson(json, alloc, scratch);

	str8 out_file_path = make_file_name_with_ext(alloc, out_path, str8_lit(PINB_EXT));

	void *out_file;
	if(!(out_file = sys_file_open_w(out_file_path))) {
		log_error("pinb-gen", "can't open file %s for writing!", out_file_path.str);
		return -1;
	}

	struct ser_writer w = {.f = out_file};
	pinb_write(&w, table);
	sys_file_close(out_file);
	sys_free(mem_buffer);
	sys_free(scratch_mem_buffer);
	log_info("pinb-gen", "%s -> %s\n", in_path.str, out_file_path.str);

	return 1;
}
