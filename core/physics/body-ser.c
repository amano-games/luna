#include "body-ser.h"
#include "dbg.h"
#include "serialize/serialize.h"
#include "str.h"
#include "sys-log.h"
#include "collisions/collisions-ser.h"

void
body_write(struct ser_writer *w, struct body *body)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("flags"));
	ser_write_i32(w, body->flags);

	ser_write_string(w, str8_lit("x"));
	ser_write_f32(w, body->p.x);
	ser_write_string(w, str8_lit("y"));
	ser_write_f32(w, body->p.y);

	ser_write_string(w, str8_lit("restitution"));
	ser_write_f32(w, body->restitution);
	ser_write_string(w, str8_lit("dynamic_friction"));
	ser_write_f32(w, body->dynamic_friction);
	ser_write_string(w, str8_lit("static_friction"));
	ser_write_f32(w, body->static_friction);
	ser_write_string(w, str8_lit("mass"));
	ser_write_f32(w, body->mass);
	ser_write_string(w, str8_lit("mass_inv"));
	ser_write_f32(w, body->mass_inv);
	ser_write_string(w, str8_lit("inertia"));
	ser_write_f32(w, body->inertia);
	ser_write_string(w, str8_lit("inertia_inv"));
	ser_write_f32(w, body->inertia_inv);
	ser_write_string(w, str8_lit("linear_damping"));
	ser_write_f32(w, body->linear_damping);
	ser_write_string(w, str8_lit("ang_damping"));
	ser_write_f32(w, body->ang_damping);

	dbg_assert(body->shapes.count > 0);
	ser_write_string(w, str8_lit("shapes"));
	col_shapes_write(w, &body->shapes);

	ser_write_end(w);
}

struct body
body_read(struct ser_reader *r, struct ser_value obj)
{
	struct body res = {0};
	struct ser_value key, value;
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("flags"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.flags = value.i32;
		} else if(str8_match(key.str, str8_lit("x"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.p.x = value.i32;
		} else if(str8_match(key.str, str8_lit("y"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.p.y = value.i32;
		} else if(str8_match(key.str, str8_lit("restitution"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.restitution = value.f32;
		} else if(str8_match(key.str, str8_lit("dynamic_friction"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.dynamic_friction = value.f32;
		} else if(str8_match(key.str, str8_lit("static_friction"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.static_friction = value.f32;
		} else if(str8_match(key.str, str8_lit("mass"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.mass = value.f32;
		} else if(str8_match(key.str, str8_lit("mass_inv"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.mass_inv = value.f32;
		} else if(str8_match(key.str, str8_lit("inertia"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.inertia = value.f32;
		} else if(str8_match(key.str, str8_lit("inertia_inv"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.inertia_inv = value.f32;
		} else if(str8_match(key.str, str8_lit("linear_damping"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.linear_damping = value.f32;
		} else if(str8_match(key.str, str8_lit("ang_damping"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.ang_damping = value.f32;
		} else if(str8_match(key.str, str8_lit("shapes"), 0)) {
			res.shapes = col_shapes_read(r, value);
		}
	}

	return res;
}
