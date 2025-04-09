#include "pinb-ser.h"
#include "arr.h"
#include "physics/body-ser.h"
#include "serialize/serialize.h"
#include "str.h"

struct pinb_sensor pinb_sensor_read(struct ser_reader *r, struct ser_value obj);
struct pinb_switch pinb_switch_value_read(struct ser_reader *r, struct ser_value obj);
struct pinb_switch_list pinb_switch_list_read(struct ser_reader *r, struct ser_value obj);
struct pinb_sfx_sequence pinb_sfx_sequence_read(struct ser_reader *r, struct ser_value obj);
struct pinb_action pinb_action_read(struct ser_reader *r, struct ser_value obj);

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

	if(entity.reactive_sprite_offset.magnitude != 0) {
		ser_write_string(w, str8_lit("reactive_sprite_offset"));
		ser_write_object(w);
		ser_write_string(w, str8_lit("delay"));
		ser_write_f32(w, entity.reactive_sprite_offset.delay);
		ser_write_string(w, str8_lit("magnitude"));
		ser_write_f32(w, entity.reactive_sprite_offset.magnitude);
		ser_write_end(w);
	}

	if(entity.reactive_animation.animation_index != 0) {
		ser_write_string(w, str8_lit("reactive_animation"));
		ser_write_object(w);
		ser_write_string(w, str8_lit("animation_index"));
		ser_write_i32(w, entity.reactive_animation.animation_index);
		ser_write_end(w);
	}

	if(entity.plunger.charge_force_max != 0) {
		ser_write_string(w, str8_lit("plunger"));
		ser_write_object(w);

		ser_write_string(w, str8_lit("charge_force_min"));
		ser_write_f32(w, entity.plunger.charge_force_min);

		ser_write_string(w, str8_lit("charge_force_max"));
		ser_write_f32(w, entity.plunger.charge_force_max);

		ser_write_string(w, str8_lit("release_force_min"));
		ser_write_f32(w, entity.plunger.release_force_min);

		ser_write_string(w, str8_lit("release_force_max"));
		ser_write_f32(w, entity.plunger.release_force_max);

		ser_write_end(w);
	}

	if(entity.flipper.velocity_scale != 0) {
		ser_write_string(w, str8_lit("flipper"));
		ser_write_object(w);

		ser_write_string(w, str8_lit("velocity_easing_function"));
		ser_write_i32(w, entity.flipper.velocity_easing_function);
		ser_write_string(w, str8_lit("velocity_radius_max"));
		ser_write_f32(w, entity.flipper.velocity_radius_max);
		ser_write_string(w, str8_lit("velocity_radius_min"));
		ser_write_f32(w, entity.flipper.velocity_radius_min);
		ser_write_string(w, str8_lit("velocity_scale"));
		ser_write_f32(w, entity.flipper.velocity_scale);

		ser_write_end(w);
	}
	if(entity.flip.type != 0) {
		ser_write_string(w, str8_lit("flip"));
		ser_write_object(w);

		ser_write_string(w, str8_lit("type"));
		ser_write_i32(w, entity.flip.type);
		ser_write_string(w, str8_lit("is_enabled"));
		ser_write_i32(w, entity.flip.is_enabled);

		ser_write_end(w);
	}

	if(entity.gravity.value != 0) {
		ser_write_string(w, str8_lit("gravity"));
		ser_write_object(w);

		ser_write_string(w, str8_lit("value"));
		ser_write_f32(w, entity.gravity.value);

		ser_write_end(w);
	}

	if(entity.animator.initial_animation != 0) {
		ser_write_string(w, str8_lit("animator"));
		ser_write_object(w);

		ser_write_string(w, str8_lit("play_on_start"));
		ser_write_i32(w, entity.animator.play_on_start);
		ser_write_string(w, str8_lit("initial_animation"));
		ser_write_i32(w, entity.animator.initial_animation);

		ser_write_end(w);
	}

	if(entity.sfx_sequences.len > 0) {
		ser_write_string(w, str8_lit("sfx_sequences"));
		ser_write_object(w);
		{
			ser_write_string(w, str8_lit("len"));
			ser_write_i32(w, entity.sfx_sequences.len);
			ser_write_string(w, str8_lit("items"));

			ser_write_array(w);
			{
				for(usize i = 0; i < entity.sfx_sequences.len; ++i) {
					ser_write_object(w);
					{
						ser_write_string(w, str8_lit("type"));
						ser_write_i32(w, entity.sfx_sequences.items[i].type);
						ser_write_string(w, str8_lit("reset_time"));
						ser_write_f32(w, entity.sfx_sequences.items[i].reset_time);
						ser_write_string(w, str8_lit("vol_min"));
						ser_write_f32(w, entity.sfx_sequences.items[i].vol_min);
						ser_write_string(w, str8_lit("vol_max"));
						ser_write_f32(w, entity.sfx_sequences.items[i].vol_max);
						ser_write_string(w, str8_lit("pitch_min"));
						ser_write_f32(w, entity.sfx_sequences.items[i].pitch_min);
						ser_write_string(w, str8_lit("pitch_max"));
						ser_write_f32(w, entity.sfx_sequences.items[i].pitch_max);
						ser_write_string(w, str8_lit("clips_len"));
						ser_write_i32(w, entity.sfx_sequences.items[i].clips_len);

						ser_write_string(w, str8_lit("clips"));
						ser_write_array(w);
						{
							for(usize j = 0; j < entity.sfx_sequences.items[i].clips_len; ++j) {
								ser_write_string(w, entity.sfx_sequences.items[i].clips[j]);
							}
						}
						ser_write_end(w);
					}
					ser_write_end(w);
				}
			}
			ser_write_end(w);
		}
		ser_write_end(w);
	}

	if(entity.actions.len > 0) {
		ser_write_string(w, str8_lit("actions"));
		ser_write_object(w);
		{
			ser_write_string(w, str8_lit("len"));
			ser_write_i32(w, entity.actions.len);
			ser_write_string(w, str8_lit("items"));

			ser_write_array(w);
			{
				for(usize i = 0; i < entity.actions.len; ++i) {
					ser_write_object(w);
					{
						ser_write_string(w, str8_lit("action_type"));
						ser_write_i32(w, entity.actions.items[i].action_type);
						ser_write_string(w, str8_lit("action_ref"));
						ser_write_i32(w, entity.actions.items[i].action_ref);
						ser_write_string(w, str8_lit("action_arg"));
						ser_write_i32(w, entity.actions.items[i].action_arg);
						ser_write_string(w, str8_lit("event_type"));
						ser_write_i32(w, entity.actions.items[i].event_type);
						ser_write_string(w, str8_lit("event_condition"));
						ser_write_i32(w, entity.actions.items[i].event_condition);
					}
					ser_write_end(w);
				}
			}
			ser_write_end(w);
		}
		ser_write_end(w);
	}

	if(entity.sensor.shape.type != COL_TYPE_NONE) {
		ser_write_string(w, str8_lit("sensor"));

		ser_write_object(w);

		ser_write_string(w, str8_lit("is_enabled"));
		ser_write_i32(w, entity.sensor.is_enabled);
		ser_write_string(w, str8_lit("shape"));
		col_shape_write(w, entity.sensor.shape);

		ser_write_end(w);
	}

	if(entity.switch_value.is_enabled) {
		ser_write_string(w, str8_lit("switch_value"));
		ser_write_object(w);
		ser_write_string(w, str8_lit("is_enabled"));
		ser_write_i32(w, entity.switch_value.is_enabled);
		ser_write_string(w, str8_lit("value"));
		ser_write_i32(w, entity.switch_value.value);
		ser_write_string(w, str8_lit("animation_on"));
		ser_write_i32(w, entity.switch_value.animation_on);
		ser_write_string(w, str8_lit("animation_off"));
		ser_write_i32(w, entity.switch_value.animation_off);
		ser_write_end(w);
	}

	if(entity.switch_list.next != 0 || entity.switch_list.next != 0) {
		ser_write_string(w, str8_lit("switch_list"));
		ser_write_object(w);
		ser_write_string(w, str8_lit("next"));
		ser_write_i32(w, entity.switch_list.next);
		ser_write_string(w, str8_lit("prev"));
		ser_write_i32(w, entity.switch_list.prev);
		ser_write_end(w);
	}

	ser_write_end(w);
}

void
pinb_physics_props_write(struct ser_writer *w, struct pinb_physics_props props)
{
	ser_write_object(w);
	ser_write_string(w, str8_lit("steps"));
	ser_write_i32(w, props.steps);
	ser_write_string(w, str8_lit("max_translation"));
	ser_write_f32(w, props.max_translation);
	ser_write_string(w, str8_lit("max_rotation"));
	ser_write_f32(w, props.max_rotation);
	ser_write_string(w, str8_lit("penetration_correction"));
	ser_write_f32(w, props.penetration_correction);
	ser_write_string(w, str8_lit("penetration_allowance"));
	ser_write_f32(w, props.penetration_allowance);
	ser_write_end(w);
}

void
pinb_flippers_props_write(struct ser_writer *w, struct pinb_flippers_props props)
{
	ser_write_object(w);
	ser_write_string(w, str8_lit("flip_velocity"));
	ser_write_f32(w, props.flip_velocity);
	ser_write_string(w, str8_lit("rotation_max_degrees"));
	ser_write_f32(w, props.rotation_max_turns);
	ser_write_string(w, str8_lit("rotation_min_degrees"));
	ser_write_f32(w, props.rotation_min_turns);
	ser_write_string(w, str8_lit("release_velocity"));
	ser_write_f32(w, props.release_velocity);
	ser_write_end(w);
}

void
pinb_table_props_write(struct ser_writer *w, struct pinb_table_props props)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("physics_props"));
	pinb_physics_props_write(w, props.physics_props);
	ser_write_string(w, str8_lit("flippers_props"));
	pinb_flippers_props_write(w, props.flippers_props);
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

	ser_write_string(w, str8_lit("entities_max_id"));
	ser_write_i32(w, pinb.entities_max_id);

	ser_write_string(w, str8_lit("props"));
	pinb_table_props_write(w, pinb.props);

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
pinb_entity_read(struct ser_reader *r, struct ser_value obj, struct alloc alloc)
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
		} else if(str8_match(key.str, str8_lit("reactive_sprite_offset"), 0)) {
			assert(value.type == SER_TYPE_OBJECT);
			struct ser_value item_key, item_value;
			while(ser_iter_object(r, value, &item_key, &item_value)) {
				assert(item_key.type == SER_TYPE_STRING);
				if(str8_match(item_key.str, str8_lit("magnitude"), 0)) {
					assert(item_value.type == SER_TYPE_F32);
					res.reactive_sprite_offset.magnitude = item_value.f32;
				} else if(str8_match(item_key.str, str8_lit("delay"), 0)) {
					assert(item_value.type == SER_TYPE_F32);
					res.reactive_sprite_offset.delay = item_value.f32;
				}
			}
		} else if(str8_match(key.str, str8_lit("reactive_animation"), 0)) {
			assert(value.type == SER_TYPE_OBJECT);
			struct ser_value item_key, item_value;
			while(ser_iter_object(r, value, &item_key, &item_value)) {
				assert(item_key.type == SER_TYPE_STRING);
				if(str8_match(item_key.str, str8_lit("animation_index"), 0)) {
					assert(item_value.type == SER_TYPE_I32);
					res.reactive_animation.animation_index = item_value.i32;
				}
			}
		} else if(str8_match(key.str, str8_lit("plunger"), 0)) {
			assert(value.type == SER_TYPE_OBJECT);
			struct ser_value plunger_key, plunger_value;
			while(ser_iter_object(r, value, &plunger_key, &plunger_value)) {
				assert(plunger_key.type == SER_TYPE_STRING);
				if(str8_match(plunger_key.str, str8_lit("charge_force_min"), 0)) {
					assert(plunger_value.type == SER_TYPE_F32);
					res.plunger.charge_force_min = plunger_value.f32;
				} else if(str8_match(plunger_key.str, str8_lit("charge_force_max"), 0)) {
					assert(plunger_value.type == SER_TYPE_F32);
					res.plunger.charge_force_max = plunger_value.f32;
				} else if(str8_match(plunger_key.str, str8_lit("release_force_min"), 0)) {
					assert(plunger_value.type == SER_TYPE_F32);
					res.plunger.release_force_min = plunger_value.f32;
				} else if(str8_match(plunger_key.str, str8_lit("release_force_max"), 0)) {
					assert(plunger_value.type == SER_TYPE_F32);
					res.plunger.release_force_max = plunger_value.f32;
				}
			}
		} else if(str8_match(key.str, str8_lit("body"), 0)) {
			assert(value.type == SER_TYPE_OBJECT);
			res.body = body_read(r, value);
		} else if(str8_match(key.str, str8_lit("sensor"), 0)) {
			assert(value.type == SER_TYPE_OBJECT);
			res.sensor = pinb_sensor_read(r, value);
		} else if(str8_match(key.str, str8_lit("switch_value"), 0)) {
			assert(value.type == SER_TYPE_OBJECT);
			res.switch_value = pinb_switch_value_read(r, value);
		} else if(str8_match(key.str, str8_lit("switch_list"), 0)) {
			assert(value.type == SER_TYPE_OBJECT);
			res.switch_list = pinb_switch_list_read(r, value);
		} else if(str8_match(key.str, str8_lit("flipper"), 0)) {
			assert(value.type == SER_TYPE_OBJECT);
			struct ser_value flipper_key, flipper_value;
			while(ser_iter_object(r, value, &flipper_key, &flipper_value)) {
				assert(flipper_key.type == SER_TYPE_STRING);
				if(str8_match(flipper_key.str, str8_lit("velocity_easing_function"), 0)) {
					res.flipper.velocity_easing_function = flipper_value.i32;
				} else if(str8_match(flipper_key.str, str8_lit("velocity_scale"), 0)) {
					res.flipper.velocity_scale = flipper_value.f32;
				} else if(str8_match(flipper_key.str, str8_lit("velocity_radius_min"), 0)) {
					res.flipper.velocity_radius_min = flipper_value.f32;
				} else if(str8_match(flipper_key.str, str8_lit("velocity_radius_max"), 0)) {
					res.flipper.velocity_radius_max = flipper_value.f32;
				}
			}
		} else if(str8_match(key.str, str8_lit("flip"), 0)) {
			assert(value.type == SER_TYPE_OBJECT);
			struct ser_value item_key, item_value;
			while(ser_iter_object(r, value, &item_key, &item_value)) {
				if(str8_match(item_key.str, str8_lit("type"), 0)) {
					res.flip.type = item_value.i32;
				} else if(str8_match(item_key.str, str8_lit("is_enabled"), 0)) {
					res.flip.is_enabled = item_value.i32;
				}
			}
		} else if(str8_match(key.str, str8_lit("gravity"), 0)) {
			assert(value.type == SER_TYPE_OBJECT);
			struct ser_value gravity_key, gravity_value;
			while(ser_iter_object(r, value, &gravity_key, &gravity_value)) {
				assert(gravity_key.type == SER_TYPE_STRING);
				if(str8_match(gravity_key.str, str8_lit("value"), 0)) {
					res.gravity.value = gravity_value.f32;
				}
			}
		} else if(str8_match(key.str, str8_lit("animator"), 0)) {
			assert(value.type == SER_TYPE_OBJECT);
			struct ser_value item_key, item_value;
			while(ser_iter_object(r, value, &item_key, &item_value)) {
				assert(item_key.type == SER_TYPE_STRING);
				if(str8_match(item_key.str, str8_lit("play_on_start"), 0)) {
					res.animator.play_on_start = item_value.i32;
				} else if(str8_match(item_key.str, str8_lit("initial_animation"), 0)) {
					res.animator.initial_animation = item_value.i32;
				}
			}
		} else if(str8_match(key.str, str8_lit("sfx_sequences"), 0)) {
			assert(value.type == SER_TYPE_OBJECT);
			struct ser_value item_key, item_value;
			while(ser_iter_object(r, value, &item_key, &item_value)) {
				assert(item_key.type == SER_TYPE_STRING);
				if(str8_match(item_key.str, str8_lit("len"), 0)) {
					res.sfx_sequences.len   = item_value.i32;
					res.sfx_sequences.items = arr_ini(item_value.i32, sizeof(*res.sfx_sequences.items), alloc);
				} else if(str8_match(item_key.str, str8_lit("items"), 0)) {
					struct ser_value sequence_value;
					while(ser_iter_array(r, item_value, &sequence_value)) {
						arr_push(res.sfx_sequences.items, pinb_sfx_sequence_read(r, sequence_value));
					}
				}
			}
		} else if(str8_match(key.str, str8_lit("actions"), 0)) {
			assert(value.type == SER_TYPE_OBJECT);
			struct ser_value item_key, item_value;
			while(ser_iter_object(r, value, &item_key, &item_value)) {
				assert(item_key.type == SER_TYPE_STRING);
				if(str8_match(item_key.str, str8_lit("len"), 0)) {
					res.actions.len   = item_value.i32;
					res.actions.items = arr_ini(item_value.i32, sizeof(*res.sfx_sequences.items), alloc);
				} else if(str8_match(item_key.str, str8_lit("items"), 0)) {
					struct ser_value sequence_value;
					while(ser_iter_array(r, item_value, &sequence_value)) {
						arr_push(res.actions.items, pinb_action_read(r, sequence_value));
					}
				}
			}
		}
	}
	return res;
}

struct pinb_physics_props
pinb_physics_props_read(struct ser_reader *r, struct ser_value obj)
{
	assert(obj.type == SER_TYPE_OBJECT);
	struct pinb_physics_props res = {0};
	struct ser_value key, value;
	while(ser_iter_object(r, obj, &key, &value)) {
		assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("steps"), 0)) {
			res.steps = value.i32;
		} else if(str8_match(key.str, str8_lit("max_translation"), 0)) {
			res.max_translation = value.f32;
		} else if(str8_match(key.str, str8_lit("max_rotation"), 0)) {
			res.max_rotation = value.f32;
		} else if(str8_match(key.str, str8_lit("penetration_correction"), 0)) {
			res.penetration_correction = value.f32;
		} else if(str8_match(key.str, str8_lit("penetration_allowance"), 0)) {
			res.penetration_allowance = value.f32;
		}
	}
	return res;
}

struct pinb_flippers_props
pinb_flippers_props_read(struct ser_reader *r, struct ser_value obj)
{
	assert(obj.type == SER_TYPE_OBJECT);
	struct pinb_flippers_props res = {0};
	struct ser_value key, value;
	while(ser_iter_object(r, obj, &key, &value)) {
		assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("flip_velocity"), 0)) {
			res.flip_velocity = value.f32;
		} else if(str8_match(key.str, str8_lit("rotation_max_degrees"), 0)) {
			res.rotation_max_turns = value.f32;
		} else if(str8_match(key.str, str8_lit("rotation_min_degrees"), 0)) {
			res.rotation_min_turns = value.f32;
		} else if(str8_match(key.str, str8_lit("release_velocity"), 0)) {
			res.release_velocity = value.f32;
		}
	}
	return res;
}

struct pinb_table_props
pinb_table_props_read(struct ser_reader *r, struct ser_value obj)
{
	assert(obj.type == SER_TYPE_OBJECT);
	struct pinb_table_props res = {0};
	struct ser_value key, value;
	while(ser_iter_object(r, obj, &key, &value)) {
		assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("physics_props"), 0)) {
			res.physics_props = pinb_physics_props_read(r, value);
		} else if(str8_match(key.str, str8_lit("flippers_props"), 0)) {
			res.flippers_props = pinb_flippers_props_read(r, value);
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
		} else if(str8_match(key.str, str8_lit("props"), 0)) {
			assert(value.type == SER_TYPE_OBJECT);
			table->props = pinb_table_props_read(r, value);
		} else if(str8_match(key.str, str8_lit("entities_count"), 0)) {
			assert(value.type == SER_TYPE_I32);
			table->entities_count = value.i32;
			table->entities       = arr_ini(table->entities_count, sizeof(*table->entities), alloc);
		} else if(str8_match(key.str, str8_lit("entities_max_id"), 0)) {
			assert(value.type == SER_TYPE_I32);
			table->entities_max_id = value.i32;
		} else if(str8_match(key.str, str8_lit("entities"), 0)) {
			assert(value.type == SER_TYPE_ARRAY);
			assert(table->entities_count > 0);
			struct ser_value val;
			usize i = 0;
			while(ser_iter_array(r, value, &val) && i < table->entities_count) {
				arr_push(table->entities, pinb_entity_read(r, val, alloc));
				i++;
			}
		}
	}
	return res;
}

struct pinb_sensor
pinb_sensor_read(struct ser_reader *r, struct ser_value obj)
{
	struct pinb_sensor res = {0};
	struct ser_value key, value;
	while(ser_iter_object(r, obj, &key, &value)) {
		assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("is_enabled"), 0)) {
			assert(value.type == SER_TYPE_I32);
			res.is_enabled = value.i32;
		} else if(str8_match(key.str, str8_lit("shape"), 0)) {
			res.shape = col_shape_read(r, value);
		}
	}

	return res;
}

struct pinb_switch
pinb_switch_value_read(struct ser_reader *r, struct ser_value obj)
{
	struct pinb_switch res = {0};
	struct ser_value key, value;
	while(ser_iter_object(r, obj, &key, &value)) {
		assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("is_enabled"), 0)) {
			assert(value.type == SER_TYPE_I32);
			res.is_enabled = value.i32;
		} else if(str8_match(key.str, str8_lit("value"), 0)) {
			assert(value.type == SER_TYPE_I32);
			res.value = value.i32;
		} else if(str8_match(key.str, str8_lit("animation_on"), 0)) {
			assert(value.type == SER_TYPE_I32);
			res.animation_on = value.i32;
		} else if(str8_match(key.str, str8_lit("animation_off"), 0)) {
			assert(value.type == SER_TYPE_I32);
			res.animation_off = value.i32;
		}
	}

	return res;
}

struct pinb_switch_list
pinb_switch_list_read(struct ser_reader *r, struct ser_value obj)
{
	struct pinb_switch_list res = {0};
	struct ser_value key, value;
	while(ser_iter_object(r, obj, &key, &value)) {
		assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("prev"), 0)) {
			assert(value.type == SER_TYPE_I32);
			res.prev = value.i32;
		} else if(str8_match(key.str, str8_lit("next"), 0)) {
			assert(value.type == SER_TYPE_I32);
			res.next = value.i32;
		}
	}

	return res;
}

struct pinb_sfx_sequence
pinb_sfx_sequence_read(struct ser_reader *r, struct ser_value obj)
{
	struct pinb_sfx_sequence res = {0};
	assert(obj.type == SER_TYPE_OBJECT);
	struct ser_value key, value;
	while(ser_iter_object(r, obj, &key, &value)) {
		assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("type"), 0)) {
			res.type = value.i32;
		} else if(str8_match(key.str, str8_lit("reset_time"), 0)) {
			res.reset_time = value.f32;
		} else if(str8_match(key.str, str8_lit("vol_min"), 0)) {
			res.vol_min = value.f32;
		} else if(str8_match(key.str, str8_lit("vol_max"), 0)) {
			res.vol_max = value.f32;
		} else if(str8_match(key.str, str8_lit("pitch_min"), 0)) {
			res.pitch_min = value.f32;
		} else if(str8_match(key.str, str8_lit("pitch_max"), 0)) {
			res.pitch_max = value.f32;
		} else if(str8_match(key.str, str8_lit("clips_len"), 0)) {
			res.clips_len = value.i32;
		} else if(str8_match(key.str, str8_lit("clips"), 0)) {
			assert(value.type == SER_TYPE_ARRAY);
			struct ser_value clip_value;
			usize i = 0;
			while(ser_iter_array(r, value, &clip_value)) {
				res.clips[i++] = clip_value.str;
			}
		}
	}
	return res;
}

struct pinb_action
pinb_action_read(struct ser_reader *r, struct ser_value obj)
{
	struct pinb_action res = {0};
	assert(obj.type == SER_TYPE_OBJECT);
	struct ser_value key, value;
	while(ser_iter_object(r, obj, &key, &value)) {
		assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("action_type"), 0)) {
			res.action_type = value.i32;
		} else if(str8_match(key.str, str8_lit("action_ref"), 0)) {
			res.action_ref = value.i32;
		} else if(str8_match(key.str, str8_lit("action_arg"), 0)) {
			res.action_arg = value.i32;
		} else if(str8_match(key.str, str8_lit("event_type"), 0)) {
			res.event_type = value.i32;
		} else if(str8_match(key.str, str8_lit("event_condition"), 0)) {
			res.event_condition = value.i32;
		}
	}
	return res;
}
