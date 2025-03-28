#include "pinb.h"
#include "arr.h"
#include "json.h"
#include "mem-arena.h"
#include "path.h"
#include "physics/physics.h"
#include "sys.h"
#include "sys-assert.h"

#define PINB_EXT "pinb"

struct pinb_response_impulse {
	f32 magnitude;
};

struct pinb_plunger {
	f32 y_initial;
	f32 charge_force_min;
	f32 charge_force_max;
	f32 release_force_min;
	f32 release_force_max;
};

struct pinb_entity {
	i32 id;
	i32 x;
	i32 y;
	i32 type;
	struct body body;
	struct pinb_plunger plunger;
	struct pinb_response_impulse impulse;
};

struct pinb_table {
	usize version;
	usize entities_count;
	struct pinb_entity *entities;
};

struct pinb_entity_res {
	struct pinb_entity entity;
	usize token_count;
};

struct pinb_rigid_body_res {
	struct body body;
	usize token_count;
};

struct pinb_col_shape_res {
	struct col_shape shapes[10];
	usize shape_count;
	usize token_count;
};

struct pinb_col_shape_res
handle_col_shape(str8 json, jsmntok_t *tokens, i32 index)
{
	struct pinb_col_shape_res res = {0};
	jsmntok_t *root               = &tokens[index];
	assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);
	for(usize i = index + 1; i < index + res.token_count; i++) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		if(json_eq(json, key, str8_lit("polygon")) == 0) {
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
			assert(value->type == JSMN_OBJECT);
			assert(value->size == 3);
			usize item_index   = i + 1;
			res.shape_count    = 1;
			res.shapes[0].type = COL_TYPE_CIR;
			for(usize j = item_index + 1; j < item_index + (value->size * 2); j++) {
				jsmntok_t *item_key   = &tokens[j];
				jsmntok_t *item_value = &tokens[j + 1];
				str8 i_key            = json_str8(json, item_key);
				str8 i_value          = json_str8(json, item_value);
				if(json_eq(json, item_key, str8_lit("x")) == 0) {
					res.shapes[0].cir.p.x = json_parse_f32(json, item_value);
					++j;
				} else if(json_eq(json, item_key, str8_lit("y")) == 0) {
					res.shapes[0].cir.p.y = json_parse_f32(json, item_value);
					++j;
				} else if(json_eq(json, item_key, str8_lit("r")) == 0) {
					res.shapes[0].cir.r = json_parse_f32(json, item_value);
					++j;
				}
			}
			i += 2 + value->size;
		}
	}

	return res;
}

struct pinb_rigid_body_res
handle_rigid_body(str8 json, jsmntok_t *tokens, i32 index)
{
	struct pinb_rigid_body_res res = {0};
	jsmntok_t *root                = &tokens[index];
	assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);
	for(usize i = index + 1; i < index + res.token_count; i++) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];
		if(json_eq(json, key, str8_lit("flags")) == 0) {
			res.body.flags = json_parse_i32(json, value);
			++i;
		} else if(json_eq(json, key, str8_lit("angular_damping")) == 0) {
			res.body.ang_damping = 1.0f - json_parse_f32(json, value);
			++i;
		} else if(json_eq(json, key, str8_lit("linear_damping")) == 0) {
			res.body.linear_damping = 1.0f - json_parse_f32(json, value);
			++i;
		} else if(json_eq(json, key, str8_lit("dynamic_friction")) == 0) {
			res.body.dynamic_friction = json_parse_f32(json, value);
			++i;
		} else if(json_eq(json, key, str8_lit("mass")) == 0) {
			res.body.mass = json_parse_f32(json, value);
			++i;
		} else if(json_eq(json, key, str8_lit("restitution")) == 0) {
			res.body.restitution = json_parse_f32(json, value);
			++i;
		} else if(json_eq(json, key, str8_lit("static_friction")) == 0) {
			res.body.static_friction = json_parse_f32(json, value);
			++i;
		} else if(json_eq(json, key, str8_lit("collision_shape")) == 0) {
			struct pinb_col_shape_res item_res = handle_col_shape(json, tokens, i + 1);
			res.body.shape                     = item_res.shapes[0];
			i += item_res.token_count;
		}
	}
	return res;
}

struct pinb_entity_res
handle_entity(str8 json, jsmntok_t *tokens, i32 index)
{
	struct pinb_entity_res res = {0};
	jsmntok_t *root            = &tokens[index];
	assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);

	for(usize i = index + 1; i < index + res.token_count; i++) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];
		if(json_eq(json, key, str8_lit("id")) == 0) {
			res.entity.id = json_parse_i32(json, value);
			++i;
		} else if(json_eq(json, key, str8_lit("type")) == 0) {
			res.entity.type = json_parse_i32(json, value);
			++i;
		} else if(json_eq(json, key, str8_lit("x")) == 0) {
			res.entity.x = json_parse_i32(json, value);
			++i;
		} else if(json_eq(json, key, str8_lit("y")) == 0) {
			res.entity.y = json_parse_i32(json, value);
			++i;
		} else if(json_eq(json, key, str8_lit("rigid_body")) == 0) {
			struct pinb_rigid_body_res item_res = handle_rigid_body(json, tokens, i + 1);
			res.entity.body                     = item_res.body;
			i += item_res.token_count;
		}
	}
	return res;
}

struct pinb_table
handle_pinbjson(str8 json, struct alloc scratch)
{
	struct pinb_table res = {0};

	jsmn_parser parser;
	jsmn_init(&parser);
	i32 token_count = jsmn_parse(&parser, (char *)json.str, json.size, NULL, 0);
	jsmn_init(&parser);
	jsmntok_t *tokens = arr_ini(token_count, sizeof(jsmntok_t), scratch);
	i32 json_res      = jsmn_parse(&parser, (char *)json.str, json.size, tokens, token_count);
	assert(json_res == token_count);

	for(usize i = 0; i < (usize)token_count; i++) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];
		if(json_eq(json, key, str8_lit("version")) == 0) {
			assert(value->type == JSMN_PRIMITIVE);
			res.version = json_parse_i32(json, value);
			++i;
		} else if(json_eq(json, key, str8_lit("entities_count")) == 0) {
			assert(value->type == JSMN_PRIMITIVE);
			res.entities_count = json_parse_i32(json, value);
			res.entities       = arr_ini(res.entities_count, sizeof(*res.entities), scratch);
			++i;
		} else if(json_eq(json, key, str8_lit("entities")) == 0) {
			assert(value->type == JSMN_ARRAY);
			for(i32 j = 0; j < value->size; j++) {
				i32 item_index  = i + 2;
				jsmntok_t *item = tokens + item_index;
				assert(item->type == JSMN_OBJECT);
				struct pinb_entity_res item_res = handle_entity(json, tokens, item_index);
				arr_push(res.entities, item_res.entity);
				i += item_res.token_count;
			}
		}
	}
	return res;
}

i32
handle_pinball_table(str8 in_path, str8 out_path, struct alloc scratch)
{
	usize mem_size = MKILOBYTE(100);
	u8 *mem_buffer = sys_alloc(NULL, mem_size);
	assert(mem_buffer != NULL);
	struct marena marena = {0};
	marena_init(&marena, mem_buffer, mem_size);
	struct alloc alloc = marena_allocator(&marena);

	str8 json = {0};
	json_load(in_path, scratch, &json);
	struct pinb_table table = handle_pinbjson(json, scratch);

	str8 out_file_path = make_file_name_with_ext(scratch, out_path, str8_lit(PINB_EXT));

	void *out_file;
	if(!(out_file = sys_file_open_w(out_file_path))) {
		log_error("pinb-gen", "can't open file %s for writing!", out_file_path.str);
		return -1;
	}

	sys_file_close(out_file);
	sys_free(mem_buffer);
	log_info("pinb-gen", "%s -> %s\n", in_path.str, out_file_path.str);

	return 1;
}
