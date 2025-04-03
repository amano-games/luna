#include "body-ser.h"
#include "str.h"

static inline void col_cir_write(struct ser_writer *w, struct col_cir cir);
static inline struct col_cir col_cir_read(struct ser_reader *r, struct ser_value value);

void
body_write(struct ser_writer *w, struct body body)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("flags"));
	ser_write_i32(w, body.flags);

	ser_write_string(w, str8_lit("x"));
	ser_write_f32(w, body.p.x);
	ser_write_string(w, str8_lit("y"));
	ser_write_f32(w, body.p.y);

	ser_write_string(w, str8_lit("restitution"));
	ser_write_f32(w, body.restitution);
	ser_write_string(w, str8_lit("dynamic_friction"));
	ser_write_f32(w, body.dynamic_friction);
	ser_write_string(w, str8_lit("static_friction"));
	ser_write_f32(w, body.static_friction);
	ser_write_string(w, str8_lit("mass"));
	ser_write_f32(w, body.mass);
	ser_write_string(w, str8_lit("linear_damping"));
	ser_write_f32(w, body.linear_damping);
	ser_write_string(w, str8_lit("ang_damping"));
	ser_write_f32(w, body.ang_damping);

	ser_write_string(w, str8_lit("shape"));
	ser_write_object(w);

	switch(body.shape.type) {
	case COL_TYPE_AABB: {
		ser_write_string(w, str8_lit("aabb"));
		ser_write_array(w);
		ser_write_f32(w, body.shape.aabb.min.x);
		ser_write_f32(w, body.shape.aabb.min.y);
		ser_write_f32(w, body.shape.aabb.max.x);
		ser_write_f32(w, body.shape.aabb.max.y);
		ser_write_end(w);
	} break;
	case COL_TYPE_CIR: {
		ser_write_string(w, str8_lit("cir"));
		col_cir_write(w, body.shape.cir);
	} break;
	case COL_TYPE_POLY: {
		ser_write_string(w, str8_lit("poly"));
		ser_write_object(w);
		ser_write_string(w, str8_lit("count"));
		ser_write_i32(w, body.shape.poly.sub_polys[0].count);
		ser_write_string(w, str8_lit("verts"));
		ser_write_array(w);
		assert(body.shape.poly.count == 1);
		for(usize i = 0; i < body.shape.poly.sub_polys[0].count; ++i) {
			ser_write_f32(w, body.shape.poly.sub_polys[0].verts[i].x);
			ser_write_f32(w, body.shape.poly.sub_polys[0].verts[i].y);
		}
		ser_write_end(w);
		ser_write_end(w);
	} break;
	case COL_TYPE_CAPSULE: {
		ser_write_string(w, str8_lit("capsule"));
		ser_write_array(w);
		col_cir_write(w, body.shape.capsule.cirs[0]);
		col_cir_write(w, body.shape.capsule.cirs[1]);
		ser_write_end(w);
	} break;
	default: {
		BAD_PATH;
	} break;
	}
	ser_write_end(w);

	ser_write_end(w);
}

struct body
body_read(struct ser_reader *r, struct ser_value obj)
{
	struct body res = {0};
	struct ser_value key, value;
	while(ser_iter_object(r, obj, &key, &value)) {
		assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("flags"), 0)) {
			assert(value.type == SER_TYPE_I32);
			res.flags = value.i32;
		} else if(str8_match(key.str, str8_lit("x"), 0)) {
			assert(value.type == SER_TYPE_F32);
			res.p.x = value.i32;
		} else if(str8_match(key.str, str8_lit("y"), 0)) {
			assert(value.type == SER_TYPE_F32);
			res.p.y = value.i32;
		} else if(str8_match(key.str, str8_lit("restitution"), 0)) {
			assert(value.type == SER_TYPE_F32);
			res.restitution = value.f32;
		} else if(str8_match(key.str, str8_lit("dynamic_friction"), 0)) {
			assert(value.type == SER_TYPE_F32);
			res.dynamic_friction = value.f32;
		} else if(str8_match(key.str, str8_lit("static_friction"), 0)) {
			assert(value.type == SER_TYPE_F32);
			res.static_friction = value.f32;
		} else if(str8_match(key.str, str8_lit("mass"), 0)) {
			assert(value.type == SER_TYPE_F32);
			res.mass = value.f32;
		} else if(str8_match(key.str, str8_lit("linear_damping"), 0)) {
			assert(value.type == SER_TYPE_F32);
			res.linear_damping = value.f32;
		} else if(str8_match(key.str, str8_lit("ang_damping"), 0)) {
			assert(value.type == SER_TYPE_F32);
			res.ang_damping = value.f32;
		} else if(str8_match(key.str, str8_lit("shape"), 0)) {
			assert(value.type == SER_TYPE_OBJECT);
			struct ser_value shape_key, shape_value;
			while(ser_iter_object(r, value, &shape_key, &shape_value)) {
				assert(shape_key.type == SER_TYPE_STRING);
				if(str8_match(shape_key.str, str8_lit("aabb"), 0)) {
					assert(shape_value.type == SER_TYPE_ARRAY);
					res.shape.type = COL_TYPE_AABB;
					struct ser_value val;
					assert(ser_iter_array(r, shape_value, &val));
					assert(val.type == SER_TYPE_F32);
					res.shape.aabb.min.x = val.f32;
					assert(ser_iter_array(r, shape_value, &val));
					assert(val.type == SER_TYPE_F32);
					res.shape.aabb.min.y = val.f32;
					assert(ser_iter_array(r, shape_value, &val));
					assert(val.type == SER_TYPE_F32);
					res.shape.aabb.max.x = val.f32;
					assert(ser_iter_array(r, shape_value, &val));
					assert(val.type == SER_TYPE_F32);
					res.shape.aabb.max.y = val.f32;
				} else if(str8_match(shape_key.str, str8_lit("cir"), 0)) {
					assert(shape_value.type == SER_TYPE_ARRAY);
					res.shape.type = COL_TYPE_CIR;
					res.shape.cir  = col_cir_read(r, shape_value);
				} else if(str8_match(shape_key.str, str8_lit("poly"), 0)) {
					assert(shape_value.type == SER_TYPE_OBJECT);
					res.shape.type       = COL_TYPE_POLY;
					res.shape.poly.count = 1;
					struct ser_value poly_key, poly_value;
					while(ser_iter_object(r, shape_value, &poly_key, &poly_value)) {
						assert(poly_key.type == SER_TYPE_STRING);
						if(str8_match(poly_key.str, str8_lit("count"), 0)) {
							assert(poly_value.type == SER_TYPE_I32);
							res.shape.poly.sub_polys[0].count = poly_value.i32;
						} else if(str8_match(poly_key.str, str8_lit("verts"), 0)) {
							assert(poly_value.type == SER_TYPE_ARRAY);
							struct ser_value val;
							usize i = 0;
							while(ser_iter_array(r, poly_value, &val)) {
								assert(val.type == SER_TYPE_F32);
								assert(i < ARRLEN(res.shape.poly.sub_polys[0].verts));
								res.shape.poly.sub_polys[0].verts[i].x = val.f32;
								assert(ser_iter_array(r, poly_value, &val));
								res.shape.poly.sub_polys[0].verts[i].y = val.f32;
								++i;
							}
							assert(res.shape.poly.sub_polys[0].count == i);
						}
					}
				} else if(str8_match(shape_key.str, str8_lit("capsule"), 0)) {
					assert(shape_value.type == SER_TYPE_ARRAY);
					res.shape.type       = COL_TYPE_CAPSULE;
					struct ser_value val = {0};
					assert(ser_iter_array(r, shape_value, &val));
					res.shape.capsule.cirs[0] = col_cir_read(r, val);
					assert(ser_iter_array(r, shape_value, &val));
					res.shape.capsule.cirs[1] = col_cir_read(r, val);
				}
			}
		}
	}

	return res;
}

static inline void
col_cir_write(struct ser_writer *w, struct col_cir cir)
{
	ser_write_array(w);
	ser_write_f32(w, cir.p.x);
	ser_write_f32(w, cir.p.y);
	ser_write_f32(w, cir.r);
	ser_write_end(w);
}

static inline struct col_cir
col_cir_read(struct ser_reader *r, struct ser_value value)
{
	assert(value.type == SER_TYPE_ARRAY);
	struct col_cir res = {0};

	struct ser_value val;
	assert(ser_iter_array(r, value, &val));
	assert(val.type == SER_TYPE_F32);
	res.p.x = val.f32;
	assert(ser_iter_array(r, value, &val));
	assert(val.type == SER_TYPE_F32);
	res.p.y = val.f32;
	assert(ser_iter_array(r, value, &val));
	assert(val.type == SER_TYPE_F32);
	res.r = val.f32;

	return res;
}
