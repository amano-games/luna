#include "collisions-ser.h"
#include "collisions.h"
#include "dbg.h"
#include "serialize/serialize-utils.h"
#include "str.h"
#include "sys-utils.h"

#define DBG_ONLY_W_FIRST

void
col_shapes_write(struct ser_writer *w, struct col_shapes *shapes)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("count"));
	ser_write_i32(w, shapes->count);

	ser_write_string(w, str8_lit("items"));
	ser_write_array(w);
	for(size i = 0; i < shapes->count; ++i) {
		col_shape_write(w, shapes->items + i);
	}
	ser_write_end(w);

	ser_write_end(w);
}

void
col_shape_write(struct ser_writer *w, struct col_shape *shape)
{
	ser_write_object(w);

	switch(shape->type) {
	case COL_TYPE_AABB: {
		ser_write_string(w, str8_lit("aabb"));
		col_aabb_write(w, shape->aabb);
	} break;
	case COL_TYPE_CIR: {
		ser_write_string(w, str8_lit("cir"));
		col_cir_write(w, shape->cir);
	} break;
	case COL_TYPE_POLY: {
		ser_write_string(w, str8_lit("poly"));
		col_poly_write(w, shape->poly);
	} break;
	case COL_TYPE_CAPSULE: {
		ser_write_string(w, str8_lit("capsule"));
		col_capsule_write(w, shape->capsule);
	} break;
	case COL_TYPE_ELLIPSIS: {
		ser_write_string(w, str8_lit("ellipsis"));
		col_ellipsis_write(w, shape->ellipsis);
	} break;
	default: {
		dbg_sentinel("col");
	} break;
	}

error:
	ser_write_end(w);
}

void
col_cir_write(struct ser_writer *w, struct col_cir col)
{
	ser_write_array(w);
	ser_write_f32(w, col.p.x);
	ser_write_f32(w, col.p.y);
	ser_write_f32(w, col.r);
	ser_write_end(w);
}

void
col_aabb_write(struct ser_writer *w, struct col_aabb col)
{
	ser_write_array(w);
	ser_write_f32(w, col.min.x);
	ser_write_f32(w, col.min.y);
	ser_write_f32(w, col.max.x);
	ser_write_f32(w, col.max.y);
	ser_write_end(w);
}

void
col_poly_write(struct ser_writer *w, struct col_poly col)
{
	ser_write_object(w);

	{
		ser_write_string(w, str8_lit("count"));
		ser_write_i32(w, col.count);

		ser_write_string(w, str8_lit("verts"));
		ser_write_array(w);
		for(size i = 0; i < col.count; ++i) {
			ser_write_v2(w, col.verts[i]);
		}
		ser_write_end(w);

		ser_write_string(w, str8_lit("norms"));
		ser_write_array(w);
		for(size i = 0; i < col.count; ++i) {
			ser_write_v2(w, col.norms[i]);
		}
		ser_write_end(w);
	}

	ser_write_end(w);
}

void
col_capsule_write(struct ser_writer *w, struct col_capsule col)
{
	ser_write_array(w);
	col_cir_write(w, col.cirs[0]);
	col_cir_write(w, col.cirs[1]);
	ser_write_end(w);
}

struct col_shapes
col_shapes_read(struct ser_reader *r, struct ser_value obj)
{
	struct col_shapes res = {0};
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	struct ser_value key, value;
	size count = 0;
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("count"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			count = value.i32;
		} else if(str8_match(key.str, str8_lit("items"), 0)) {
			dbg_assert(value.type == SER_TYPE_ARRAY);
			struct ser_value item_value;
			while(ser_iter_array(r, value, &item_value)) {
				res.items[res.count++] = col_shape_read(r, item_value);
			}
		}
	}
	dbg_assert(count == res.count);
	return res;
}

struct col_shape
col_shape_read(struct ser_reader *r, struct ser_value obj)
{
	struct col_shape res = {0};
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	struct ser_value key, value;
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("aabb"), 0)) {
			res.type = COL_TYPE_AABB;
			res.aabb = col_aabb_read(r, value);
		} else if(str8_match(key.str, str8_lit("cir"), 0)) {
			res.type = COL_TYPE_CIR;
			res.cir  = col_cir_read(r, value);
		} else if(str8_match(key.str, str8_lit("poly"), 0)) {
			res.type = COL_TYPE_POLY;
			res.poly = col_poly_read(r, value);
		} else if(str8_match(key.str, str8_lit("capsule"), 0)) {
			res.type    = COL_TYPE_CAPSULE;
			res.capsule = col_capsule_read(r, value);
		} else if(str8_match(key.str, str8_lit("ellipsis"), 0)) {
			res.type     = COL_TYPE_ELLIPSIS;
			res.ellipsis = col_ellipsis_read(r, value);
		}
	}
	return res;
}

struct col_cir
col_cir_read(struct ser_reader *r, struct ser_value arr)
{
	dbg_assert(arr.type == SER_TYPE_ARRAY);
	struct col_cir res     = {0};
	struct ser_value value = {0};

	ser_iter_array(r, arr, &value);
	dbg_assert(value.type == SER_TYPE_F32);
	res.p.x = value.f32;

	ser_iter_array(r, arr, &value);
	dbg_assert(value.type == SER_TYPE_F32);
	res.p.y = value.f32;

	ser_iter_array(r, arr, &value);
	dbg_assert(value.type == SER_TYPE_F32);
	res.r = value.f32;

	return res;
}

struct col_aabb
col_aabb_read(struct ser_reader *r, struct ser_value arr)
{
	struct col_aabb res    = {0};
	struct ser_value value = {0};
	dbg_assert(arr.type == SER_TYPE_ARRAY);

	ser_iter_array(r, arr, &value);
	dbg_assert(value.type == SER_TYPE_F32);
	res.min.x = value.f32;

	ser_iter_array(r, arr, &value);
	dbg_assert(value.type == SER_TYPE_F32);
	res.min.y = value.f32;

	ser_iter_array(r, arr, &value);
	dbg_assert(value.type == SER_TYPE_F32);
	res.max.x = value.f32;

	ser_iter_array(r, arr, &value);
	dbg_assert(value.type == SER_TYPE_F32);
	res.max.y = value.f32;

	return res;
}

struct col_poly
col_poly_read(struct ser_reader *r, struct ser_value obj)
{
	struct col_poly res = {0};
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	struct ser_value key, value;

	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("count"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.count = value.i32;
		} else if(str8_match(key.str, str8_lit("verts"), 0)) {
			dbg_assert(value.type == SER_TYPE_ARRAY);
			struct ser_value item_value;
			size i = 0;
			while(ser_iter_array(r, value, &item_value)) {
				dbg_assert(i < (size)ARRLEN(res.verts));
				res.verts[i++] = ser_read_v2(r, item_value);
			}
			dbg_assert(res.count == i);
		} else if(str8_match(key.str, str8_lit("norms"), 0)) {
			dbg_assert(value.type == SER_TYPE_ARRAY);
			struct ser_value item_value;
			size i = 0;
			while(ser_iter_array(r, value, &item_value)) {
				dbg_assert(i < (size)ARRLEN(res.norms));
				res.norms[i++] = ser_read_v2(r, item_value);
			}
			dbg_assert(res.count == i);
		}
	}

	return res;
}

struct col_capsule
col_capsule_read(struct ser_reader *r, struct ser_value arr)
{
	dbg_assert(arr.type == SER_TYPE_ARRAY);
	struct col_capsule res = {0};
	struct ser_value value = {0};

	ser_iter_array(r, arr, &value);
	res.cirs[0] = col_cir_read(r, value);

	ser_iter_array(r, arr, &value);
	res.cirs[1] = col_cir_read(r, value);

	return res;
}

void
col_ellipsis_write(struct ser_writer *w, struct col_ellipsis value)
{
	ser_write_array(w);
	ser_write_f32(w, value.p.x);
	ser_write_f32(w, value.p.y);
	ser_write_f32(w, value.rx);
	ser_write_f32(w, value.ry);
	ser_write_end(w);
}

struct col_ellipsis
col_ellipsis_read(struct ser_reader *r, struct ser_value arr)
{
	dbg_assert(arr.type == SER_TYPE_ARRAY);
	struct col_ellipsis res = {0};
	struct ser_value value  = {0};

	ser_iter_array(r, arr, &value);
	dbg_assert(value.type == SER_TYPE_F32);
	res.p.x = value.f32;

	ser_iter_array(r, arr, &value);
	dbg_assert(value.type == SER_TYPE_F32);
	res.p.y = value.f32;

	ser_iter_array(r, arr, &value);
	dbg_assert(value.type == SER_TYPE_F32);
	res.rx = value.f32;

	ser_iter_array(r, arr, &value);
	dbg_assert(value.type == SER_TYPE_F32);
	res.ry = value.f32;

	return res;
}

void
col_line_write(struct ser_writer *w, struct col_line value)
{
	ser_write_array(w);
	ser_write_f32(w, value.a.x);
	ser_write_f32(w, value.a.y);
	ser_write_f32(w, value.b.x);
	ser_write_f32(w, value.b.y);
	ser_write_end(w);
}

struct col_line
col_line_read(struct ser_reader *r, struct ser_value arr)
{
	dbg_assert(arr.type == SER_TYPE_ARRAY);
	struct col_line res    = {0};
	struct ser_value value = {0};

	ser_iter_array(r, arr, &value);
	dbg_assert(value.type == SER_TYPE_F32);
	res.a.x = value.f32;

	ser_iter_array(r, arr, &value);
	dbg_assert(value.type == SER_TYPE_F32);
	res.a.y = value.f32;

	ser_iter_array(r, arr, &value);
	dbg_assert(value.type == SER_TYPE_F32);
	res.b.x = value.f32;

	ser_iter_array(r, arr, &value);
	dbg_assert(value.type == SER_TYPE_F32);
	res.b.y = value.f32;

	return res;
}
