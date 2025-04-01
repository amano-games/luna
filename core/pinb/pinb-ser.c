#include "pinb-ser.h"
#include "arr.h"
#include "physics/body-ser.h"
#include "serialize/serialize.h"
#include "str.h"

void
pinb_entity_write(struct ser_writer *w, struct pinb_entity entity)
{
	ser_write_object(w);
	ser_write_string(w, str8_lit("id"));
	ser_write_i32(w, entity.id);
	ser_write_string(w, str8_lit("x"));
	ser_write_i32(w, entity.x);
	ser_write_string(w, str8_lit("y"));
	ser_write_i32(w, entity.y);
	ser_write_string(w, str8_lit("type"));
	ser_write_i32(w, entity.type);
	if(entity.spr.path.size > 0) {
		ser_write_string(w, str8_lit("spr"));
		ser_write_object(w);

		ser_write_string(w, str8_lit("path"));
		ser_write_string(w, entity.spr.path);
		ser_write_string(w, str8_lit("flip"));
		ser_write_i32(w, entity.spr.flip);
		ser_write_string(w, str8_lit("offset"));
		ser_write_array(w);
		ser_write_f32(w, entity.spr.offset.x);
		ser_write_f32(w, entity.spr.offset.y);
		ser_write_end(w);

		ser_write_end(w);
	}
	if(entity.body.shape.type != COL_TYPE_NONE) {
		ser_write_string(w, str8_lit("body"));
		body_write(w, entity.body);
	}
	if(entity.reactive_impulse.magnitude != 0) {
		ser_write_string(w, str8_lit("reactive_impulse"));
		ser_write_object(w);
		ser_write_string(w, str8_lit("magnitude"));
		ser_write_f32(w, entity.reactive_impulse.magnitude);
		ser_write_string(w, str8_lit("normalize"));
		ser_write_i32(w, entity.reactive_impulse.normalize);
		ser_write_end(w);
	}
	ser_write_end(w);
}

i32
pinb_write(struct ser_writer *w, struct pinb_table pinb)
{
	i32 res = 0;
	ser_write_object(w);

	ser_write_string(w, str8_lit("version"));
	ser_write_i32(w, pinb.version);
	ser_write_string(w, str8_lit("entities_count"));
	ser_write_i32(w, pinb.entities_count);

	ser_write_string(w, str8_lit("entities"));
	ser_write_array(w);
	for(usize i = 0; i < pinb.entities_count; ++i) {
		pinb_entity_write(w, pinb.entities[i]);
	}
	ser_write_end(w);

	ser_write_end(w);
	return res;
}

struct pinb_entity
pinb_entity_read(struct ser_reader *r, struct ser_value obj)
{
	assert(obj.type == SER_TYPE_OBJECT);
	struct pinb_entity res = {0};
	struct ser_value key, value;
	while(ser_iter_object(r, obj, &key, &value)) {
		assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("id"), 0)) {
			assert(value.type == SER_TYPE_I32);
			res.id = value.i32;
		} else if(str8_match(key.str, str8_lit("x"), 0)) {
			assert(value.type == SER_TYPE_I32);
			res.x = value.i32;
		} else if(str8_match(key.str, str8_lit("y"), 0)) {
			assert(value.type == SER_TYPE_I32);
			res.y = value.i32;
		} else if(str8_match(key.str, str8_lit("type"), 0)) {
			assert(value.type == SER_TYPE_I32);
			res.type = value.i32;
		} else if(str8_match(key.str, str8_lit("spr"), 0)) {
			assert(value.type == SER_TYPE_OBJECT);
			struct ser_value spr_key, spr_value;
			while(ser_iter_object(r, value, &spr_key, &spr_value)) {
				assert(spr_key.type == SER_TYPE_STRING);
				if(str8_match(spr_key.str, str8_lit("path"), 0)) {
					assert(spr_value.type == SER_TYPE_STRING);
					res.spr.path = spr_value.str;
				} else if(str8_match(spr_key.str, str8_lit("flip"), 0)) {
					assert(spr_value.type == SER_TYPE_I32);
					res.spr.flip = spr_value.i32;
				} else if(str8_match(spr_key.str, str8_lit("offset"), 0)) {
					assert(spr_value.type == SER_TYPE_ARRAY);
					struct ser_value offset_value;
					ser_iter_array(r, spr_value, &offset_value);
					assert(offset_value.type == SER_TYPE_F32);
					res.spr.offset.x = offset_value.f32;
					ser_iter_array(r, spr_value, &offset_value);
					assert(offset_value.type == SER_TYPE_F32);
					res.spr.offset.y = offset_value.f32;
				}
			}
		} else if(str8_match(key.str, str8_lit("reactive_impulse"), 0)) {
			assert(value.type == SER_TYPE_OBJECT);
			struct ser_value reactive_impulse_key, reactive_impulse_value;
			while(ser_iter_object(r, value, &reactive_impulse_key, &reactive_impulse_value)) {
				assert(reactive_impulse_key.type == SER_TYPE_STRING);
				if(str8_match(reactive_impulse_key.str, str8_lit("magnitude"), 0)) {
					assert(reactive_impulse_value.type == SER_TYPE_F32);
					res.reactive_impulse.magnitude = reactive_impulse_value.f32;
				} else if(str8_match(reactive_impulse_key.str, str8_lit("normalize"), 0)) {
					assert(reactive_impulse_value.type == SER_TYPE_I32);
					res.reactive_impulse.normalize = reactive_impulse_value.i32;
				}
			}
		} else if(str8_match(key.str, str8_lit("body"), 0)) {
			assert(value.type == SER_TYPE_OBJECT);
			res.body = body_read(r, value);
		}
	}
	return res;
}

i32
pinb_read(struct ser_reader *r, struct ser_value obj, struct pinb_table *table, struct alloc alloc)
{
	i32 res = 0;
	struct ser_value key, value;
	while(ser_iter_object(r, obj, &key, &value)) {
		assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("version"), 0)) {
			assert(value.type == SER_TYPE_I32);
			assert(value.i32 > 0);
			table->version = value.i32;
		} else if(str8_match(key.str, str8_lit("entities_count"), 0)) {
			assert(value.type == SER_TYPE_I32);
			table->entities_count = value.i32;
			table->entities       = arr_ini(table->entities_count, sizeof(*table->entities), alloc);
		} else if(str8_match(key.str, str8_lit("entities"), 0)) {
			assert(value.type == SER_TYPE_ARRAY);
			assert(table->entities_count > 0);
			struct ser_value val;
			usize i = 0;
			while(ser_iter_array(r, value, &val) && i < table->entities_count) {
				arr_push(table->entities, pinb_entity_read(r, val));
				i++;
			}
		}
	}
	return res;
}
