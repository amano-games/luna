#include "pinb-ser.h"
#include "arr.h"
#include "physics/body-ser.h"
#include "pinb/pinb.h"
#include "serialize/serialize-utils.h"
#include "serialize/serialize.h"
#include "str.h"
#include "sys-log.h"
#include "collisions/collisions-ser.h"

i32
pinb_read(
	struct ser_reader *r,
	struct pinb_table *table,
	struct alloc alloc)
{
	i32 res               = 0;
	struct ser_value root = ser_read(r);
	struct ser_value key, value;
	while(ser_iter_object(r, root, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("version"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			dbg_assert(value.i32 > 0);
			table->version = value.i32;
			dbg_assert(table->version == 1);
		} else if(str8_match(key.str, str8_lit("props"), 0)) {
			dbg_assert(value.type == SER_TYPE_OBJECT);
			table->props = pinb_table_props_read(r, value);
		} else if(str8_match(key.str, str8_lit("entities_count"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			table->entities_count = value.i32;
			table->entities       = arr_new(table->entities, table->entities_count, alloc);
		} else if(str8_match(key.str, str8_lit("entities_max_id"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			table->entities_max_id = value.i32;
		} else if(str8_match(key.str, str8_lit("entities"), 0)) {
			dbg_assert(value.type == SER_TYPE_ARRAY);
			struct ser_value val;
			while(ser_iter_array(r, value, &val)) {
				arr_push(table->entities, pinb_entity_read(r, val, alloc));
			}
			dbg_assert(arr_len(table->entities) == table->entities_count);
		}
	}
	log_info(
		"Pinb",
		"Parsed, version: %d\n"
		"entities_count: %d\n"
		"entities_max_id %d",
		table->version,
		table->entities_count,
		table->entities_max_id);

	return res;
}

i32
pinb_inspect(str8 path, struct ser_reader *r, struct alloc alloc)
{
	i32 res               = 0;
	struct str8_list list = {0};
	struct ser_value root = ser_read(r);
	ser_value_push(r, root, 0, &list, alloc);
	str8 str          = str8_list_join(alloc, &list, NULL);
	void *inspct_file = NULL;
	if(!(inspct_file = sys_file_open_w(path))) {
		log_error("pinb-ser", "can't open file %s for writing!", path.str);
		return -1;
	}
	sys_file_w(inspct_file, str.str, str.size);
	sys_file_close(inspct_file);
	log_info("pinb-ser", "inspect output: %s", path.str);

	return res;
}

void
pinb_entity_write(struct ser_writer *w, struct pinb_entity entity)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("id"));
	ser_write_i32(w, entity.id);
	ser_write_string(w, str8_lit("flags"));
	ser_write_i32(w, entity.flags);
	ser_write_string(w, str8_lit("x"));
	ser_write_i32(w, entity.x);
	ser_write_string(w, str8_lit("y"));
	ser_write_i32(w, entity.y);

	if(entity.spr.path.size > 0) {
		ser_write_string(w, str8_lit("spr"));
		pinb_entity_spr_write(w, entity);
	}

	if(entity.body.shapes.count > 0) {
		ser_write_string(w, str8_lit("body"));
		body_write(w, entity.body);
	}

	if(entity.sensor.shapes.count > 0) {
		ser_write_string(w, str8_lit("sensor"));
		pinb_sensor_write(w, entity.sensor);
	}

	if(entity.reactive_impulse.magnitude != 0) {
		ser_write_string(w, str8_lit("reactive_impulse"));
		pinb_reactive_impulse_write(w, entity.reactive_impulse);
	}

	if(entity.force_field.magnitude != 0) {
		ser_write_string(w, str8_lit("force_field"));
		pinb_force_field_write(w, entity.force_field);
	}

	if(entity.attractor.force != 0) {
		ser_write_string(w, str8_lit("attractor"));
		pinb_attractor_write(w, entity.attractor);
	}

	if(entity.reactive_sprite_offset.magnitude != 0) {
		ser_write_string(w, str8_lit("reactive_sprite_offset"));
		pinb_reactive_sprite_offset_write(w, entity.reactive_sprite_offset);
	}

	if(entity.reactive_animation.animation_index != 0) {
		ser_write_string(w, str8_lit("reactive_animation"));
		pinb_reactive_animation_write(w, entity.reactive_animation);
	}

	if(entity.charged_impulse.magnitude != 0) {
		ser_write_string(w, str8_lit("charged_impulse"));
		pinb_charged_impulse_write(w, entity.charged_impulse);
	}

	if(entity.plunger.charge_force_max != 0) {
		ser_write_string(w, str8_lit("plunger"));
		pinb_plunger_write(w, entity.plunger);
	}

	if(entity.spinner.spin_force != 0) {
		ser_write_string(w, str8_lit("spinner"));
		pinb_spinner_write(w, entity.spinner);
	}

	if(entity.bucket.impulse_magnitude != 0) {
		ser_write_string(w, str8_lit("bucket"));
		pinb_bucket_write(w, entity.bucket);
	}

	if(entity.ball_saver.duration > 0) {
		ser_write_string(w, str8_lit("ball_saver"));
		pinb_ball_saver_write(w, entity.ball_saver);
	}

	if(entity.flipper.velocity_scale != 0) {
		ser_write_string(w, str8_lit("flipper"));
		pinb_flipper_write(w, entity.flipper);
	}

	if(entity.flip.type != 0) {
		ser_write_string(w, str8_lit("flip"));
		pinb_flip_write(w, entity.flip);
	}

	if(entity.gravity.value != 0) {
		ser_write_string(w, str8_lit("gravity"));
		pinb_gravity_write(w, entity.gravity);
	}

	if(entity.counter.resolution != 0) {
		ser_write_string(w, str8_lit("counter"));
		pinb_counter_write(w, entity.counter);
	}

	if(entity.collision_layer.layer > 0) {
		ser_write_string(w, str8_lit("collision_layer"));
		pinb_collision_layer_write(w, entity.collision_layer);
	}

	if(entity.crank_animation.interval != 0) {
		ser_write_string(w, str8_lit("crank_animation"));
		pinb_crank_animation_write(w, entity.crank_animation);
	}

	if(entity.reset.flags != 0) {
		ser_write_string(w, str8_lit("reset"));
		pinb_reset_write(w, entity.reset);
	}

	if(entity.animator.initial_animation != 0) {
		ser_write_string(w, str8_lit("animator"));
		pinb_animator_write(w, entity.animator);
	}

	if(entity.score_fx_offset.x != 0 || entity.score_fx_offset.y != 0) {
		ser_write_string(w, str8_lit("score_fx_offset"));
		ser_write_v2_i32(w, entity.score_fx_offset);
	}

	if(entity.switch_value.is_enabled) {
		ser_write_string(w, str8_lit("switch_value"));
		pinb_switch_value_write(w, entity.switch_value);
	}

	if(entity.switch_list.next != 0 || entity.switch_list.next != 0) {
		ser_write_string(w, str8_lit("switch_list"));
		pinb_switch_list_write(w, entity.switch_list);
	}

	if(entity.spawner.ref != 0) {
		ser_write_string(w, str8_lit("spawner"));
		pinb_spawner_write(w, entity.spawner);
	}

	if(entity.sfx_sequences.len > 0) {
		ser_write_string(w, str8_lit("sfx_sequences"));
		pinb_sfx_sequences_write(w, entity.sfx_sequences);
	}

	if(entity.messages.len > 0) {
		ser_write_string(w, str8_lit("messages"));
		pinb_messages_write(w, entity.messages);
	}

	if(entity.actions.len > 0) {
		ser_write_string(w, str8_lit("actions"));
		pinb_actions_write(w, entity.actions);
	}

	ser_write_end(w);
}

struct pinb_entity
pinb_entity_read(struct ser_reader *r, struct ser_value obj, struct alloc alloc)
{
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	struct pinb_entity res = {0};
	struct ser_value key, value;
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("id"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.id = value.i32;
		} else if(str8_match(key.str, str8_lit("flags"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.flags = value.i32;
		} else if(str8_match(key.str, str8_lit("x"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.x = value.i32;
		} else if(str8_match(key.str, str8_lit("y"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.y = value.i32;
		} else if(str8_match(key.str, str8_lit("spr"), 0)) {
			res.spr = pinb_spr_read(r, value);
		} else if(str8_match(key.str, str8_lit("reactive_impulse"), 0)) {
			res.reactive_impulse = pinb_reactive_impulse_read(r, value);
		} else if(str8_match(key.str, str8_lit("force_field"), 0)) {
			res.force_field = pinb_force_field_read(r, value);
		} else if(str8_match(key.str, str8_lit("attractor"), 0)) {
			res.attractor = pinb_attractor_read(r, value);
		} else if(str8_match(key.str, str8_lit("reactive_sprite_offset"), 0)) {
			res.reactive_sprite_offset = pinb_reactive_sprite_offset_read(r, value);
		} else if(str8_match(key.str, str8_lit("reactive_animation"), 0)) {
			res.reactive_animation = pinb_reactive_animation_read(r, value);
		} else if(str8_match(key.str, str8_lit("charged_impulse"), 0)) {
			res.charged_impulse = pinb_charged_impulse_read(r, value);
		} else if(str8_match(key.str, str8_lit("plunger"), 0)) {
			res.plunger = pinb_plunger_read(r, value);
		} else if(str8_match(key.str, str8_lit("spinner"), 0)) {
			res.spinner = pinb_spinner_read(r, value);
		} else if(str8_match(key.str, str8_lit("bucket"), 0)) {
			res.bucket = pinb_bucket_read(r, value);
		} else if(str8_match(key.str, str8_lit("ball_saver"), 0)) {
			res.ball_saver = pinb_ball_saver_read(r, value);
		} else if(str8_match(key.str, str8_lit("body"), 0)) {
			res.body = body_read(r, value);
		} else if(str8_match(key.str, str8_lit("sensor"), 0)) {
			res.sensor = pinb_sensor_read(r, value);
		} else if(str8_match(key.str, str8_lit("switch_value"), 0)) {
			res.switch_value = pinb_switch_value_read(r, value);
		} else if(str8_match(key.str, str8_lit("switch_list"), 0)) {
			res.switch_list = pinb_switch_list_read(r, value);
		} else if(str8_match(key.str, str8_lit("flipper"), 0)) {
			res.flipper = pinb_flipper_read(r, value);
		} else if(str8_match(key.str, str8_lit("flip"), 0)) {
			res.flip = pinb_flip_read(r, value);
		} else if(str8_match(key.str, str8_lit("gravity"), 0)) {
			res.gravity = pinb_gravity_read(r, value);
		} else if(str8_match(key.str, str8_lit("counter"), 0)) {
			res.counter = pinb_counter_read(r, value);
		} else if(str8_match(key.str, str8_lit("collision_layer"), 0)) {
			res.collision_layer = pinb_collision_layer_read(r, value);
		} else if(str8_match(key.str, str8_lit("crank_animation"), 0)) {
			res.crank_animation = pinb_crank_animation_read(r, value);
		} else if(str8_match(key.str, str8_lit("reset"), 0)) {
			res.reset = pinb_reset_read(r, value);
		} else if(str8_match(key.str, str8_lit("score_fx_offset"), 0)) {
			res.score_fx_offset = ser_read_v2_i32(r, value);
		} else if(str8_match(key.str, str8_lit("animator"), 0)) {
			res.animator = pinb_animator_read(r, value, alloc);
		} else if(str8_match(key.str, str8_lit("spawner"), 0)) {
			res.spawner = pinb_spawner_read(r, value);
		} else if(str8_match(key.str, str8_lit("sfx_sequences"), 0)) {
			res.sfx_sequences = pinb_sfx_sequences_read(r, value, alloc);
		} else if(str8_match(key.str, str8_lit("messages"), 0)) {
			res.messages = pinb_messages_read(r, value, alloc);
		} else if(str8_match(key.str, str8_lit("actions"), 0)) {
			res.actions = pinb_actions_read(r, value, alloc);
		}
	}
	return res;
}

void
pinb_counter_write(struct ser_writer *w, struct pinb_counter value)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("min"));
	ser_write_i32(w, value.min);
	ser_write_string(w, str8_lit("max"));
	ser_write_i32(w, value.max);
	ser_write_string(w, str8_lit("value"));
	ser_write_i32(w, value.value);
	ser_write_string(w, str8_lit("resolution"));
	ser_write_i32(w, value.resolution);

	ser_write_end(w);
}

void
pinb_sensor_write(struct ser_writer *w, struct pinb_sensor value)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("is_enabled"));
	ser_write_i32(w, value.is_enabled);

	ser_write_string(w, str8_lit("shapes"));
	col_shapes_write(w, value.shapes);

	ser_write_end(w);
}

void
pinb_reactive_impulse_write(struct ser_writer *w, struct pinb_reactive_impulse value)
{
	ser_write_object(w);
	ser_write_string(w, str8_lit("magnitude"));
	ser_write_f32(w, value.magnitude);
	ser_write_string(w, str8_lit("normalize"));
	ser_write_i32(w, value.normalize);
	ser_write_end(w);
}

void
pinb_force_field_write(struct ser_writer *w, struct pinb_force_field value)
{
	ser_write_object(w);
	ser_write_string(w, str8_lit("magnitude"));
	ser_write_f32(w, value.magnitude);
	ser_write_string(w, str8_lit("angle"));
	ser_write_f32(w, value.angle);
	ser_write_string(w, str8_lit("is_enabled"));
	ser_write_i32(w, value.is_enabled);
	ser_write_end(w);
}

void
pinb_attractor_write(struct ser_writer *w, struct pinb_attractor value)
{
	ser_write_object(w);
	ser_write_string(w, str8_lit("flags"));
	ser_write_i32(w, value.flags);
	ser_write_string(w, str8_lit("offset"));
	ser_write_v2(w, value.offset);
	ser_write_string(w, str8_lit("radius"));
	ser_write_f32(w, value.radius);
	ser_write_string(w, str8_lit("force"));
	ser_write_f32(w, value.force);
	ser_write_string(w, str8_lit("damping"));
	ser_write_f32(w, value.damping);
	ser_write_string(w, str8_lit("distance_threshold"));
	ser_write_f32(w, value.distance_threshold);
	ser_write_end(w);
}

void
pinb_reactive_sprite_offset_write(struct ser_writer *w, struct pinb_reactive_sprite_offset value)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("delay"));
	ser_write_f32(w, value.delay);
	ser_write_string(w, str8_lit("magnitude"));
	ser_write_f32(w, value.magnitude);
	ser_write_string(w, str8_lit("ref"));
	ser_write_i32(w, value.ref);

	ser_write_end(w);
}

void
pinb_reactive_animation_write(struct ser_writer *w, struct pinb_reactive_animation value)
{
	ser_write_object(w);
	ser_write_string(w, str8_lit("animation_index"));
	ser_write_i32(w, value.animation_index);
	ser_write_end(w);
}

void
pinb_charged_impulse_write(struct ser_writer *w, struct pinb_charged_impulse value)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("angle"));
	ser_write_f32(w, value.angle);
	ser_write_string(w, str8_lit("magnitude"));
	ser_write_f32(w, value.magnitude);
	ser_write_string(w, str8_lit("charge_speed"));
	ser_write_f32(w, value.charge_speed);
	ser_write_string(w, str8_lit("release_speed"));
	ser_write_f32(w, value.release_speed);
	ser_write_string(w, str8_lit("reset_target"));
	ser_write_i32(w, value.reset_target);
	ser_write_string(w, str8_lit("auto_shoot"));
	ser_write_i32(w, value.auto_shoot);

	ser_write_end(w);
}

void
pinb_plunger_write(struct ser_writer *w, struct pinb_plunger value)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("charge_force_min"));
	ser_write_f32(w, value.charge_force_min);

	ser_write_string(w, str8_lit("charge_force_max"));
	ser_write_f32(w, value.charge_force_max);

	ser_write_string(w, str8_lit("release_force_min"));
	ser_write_f32(w, value.release_force_min);

	ser_write_string(w, str8_lit("release_force_max"));
	ser_write_f32(w, value.release_force_max);

	ser_write_end(w);
}

void
pinb_spinner_write(struct ser_writer *w, struct pinb_spinner value)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("damping"));
	ser_write_f32(w, value.damping);

	ser_write_string(w, str8_lit("spin_force"));
	ser_write_f32(w, value.spin_force);

	ser_write_string(w, str8_lit("stop_threshold"));
	ser_write_f32(w, value.stop_threshold);

	ser_write_end(w);
}

void
pinb_bucket_write(struct ser_writer *w, struct pinb_bucket value)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("animation_shoot"));
	ser_write_i32(w, value.animation_shoot);

	ser_write_string(w, str8_lit("animation_on"));
	ser_write_i32(w, value.animation_on);

	ser_write_string(w, str8_lit("animation_off"));
	ser_write_i32(w, value.animation_off);

	ser_write_string(w, str8_lit("impulse_magnitude"));
	ser_write_f32(w, value.impulse_magnitude);

	ser_write_string(w, str8_lit("impulse_angle"));
	ser_write_f32(w, value.impulse_angle);

	ser_write_string(w, str8_lit("delay"));
	ser_write_f32(w, value.delay);

	ser_write_end(w);
}

void
pinb_ball_saver_write(struct ser_writer *w, struct pinb_ball_saver value)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("is_enabled"));
	ser_write_i32(w, value.is_enabled);

	ser_write_string(w, str8_lit("duration"));
	ser_write_f32(w, value.duration);

	ser_write_string(w, str8_lit("save_delay"));
	ser_write_f32(w, value.save_delay);

	ser_write_end(w);
}

void
pinb_flipper_write(struct ser_writer *w, struct pinb_flipper value)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("velocity_easing_function"));
	ser_write_i32(w, value.velocity_easing_function);
	ser_write_string(w, str8_lit("velocity_radius_max"));
	ser_write_f32(w, value.velocity_radius_max);
	ser_write_string(w, str8_lit("velocity_radius_min"));
	ser_write_f32(w, value.velocity_radius_min);
	ser_write_string(w, str8_lit("velocity_scale"));
	ser_write_f32(w, value.velocity_scale);

	ser_write_end(w);
}

void
pinb_flip_write(struct ser_writer *w, struct pinb_flip value)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("type"));
	ser_write_i32(w, value.type);
	ser_write_string(w, str8_lit("is_enabled"));
	ser_write_i32(w, value.is_enabled);

	ser_write_end(w);
}

void
pinb_gravity_write(struct ser_writer *w, struct pinb_gravity value)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("value"));
	ser_write_f32(w, value.value);

	ser_write_end(w);
}

void
pinb_collision_layer_write(struct ser_writer *w, struct pinb_collision_layer value)
{
	ser_write_object(w);
	ser_write_string(w, str8_lit("layer"));
	ser_write_i32(w, value.layer);
	ser_write_end(w);
}

void
pinb_crank_animation_write(struct ser_writer *w, struct pinb_crank_animation value)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("interval"));
	ser_write_f32(w, value.interval);

	ser_write_end(w);
}

void
pinb_reset_write(struct ser_writer *w, struct pinb_reset value)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("flags"));
	ser_write_i32(w, value.flags);

	ser_write_end(w);
}

void
pinb_animator_write(struct ser_writer *w, struct pinb_animator value)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("play_on_start"));
	ser_write_i32(w, value.play_on_start);
	ser_write_string(w, str8_lit("initial_animation"));
	ser_write_i32(w, value.initial_animation);

	if(value.transitions.len > 0) {
		ser_write_string(w, str8_lit("transitions"));
		pinb_animator_transitions_write(w, value.transitions);
	}
	ser_write_end(w);
}

void
pinb_animator_transitions_write(struct ser_writer *w, struct pinb_animator_transitions value)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("len"));
	ser_write_i32(w, value.len);

	ser_write_string(w, str8_lit("items"));
	ser_write_array(w);
	for(size i = 0; i < value.len; ++i) {
		ser_write_array(w);
		ser_write_i32(w, value.items[i].from);
		ser_write_i32(w, value.items[i].to);
		ser_write_end(w);
	}
	ser_write_end(w);
	ser_write_end(w);
}

void
pinb_switch_value_write(struct ser_writer *w, struct pinb_switch value)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("is_enabled"));
	ser_write_i32(w, value.is_enabled);
	ser_write_string(w, str8_lit("value"));
	ser_write_i32(w, value.value);
	ser_write_string(w, str8_lit("animation_on"));
	ser_write_i32(w, value.animation_on);
	ser_write_string(w, str8_lit("animation_off"));
	ser_write_i32(w, value.animation_off);

	ser_write_end(w);
}

void
pinb_switch_list_write(struct ser_writer *w, struct pinb_switch_list value)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("next"));
	ser_write_i32(w, value.next);
	ser_write_string(w, str8_lit("prev"));
	ser_write_i32(w, value.prev);

	ser_write_end(w);
}

void
pinb_spawner_write(struct ser_writer *w, struct pinb_spawner value)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("offset"));
	ser_write_v2_i32(w, value.offset);
	ser_write_string(w, str8_lit("ref"));
	ser_write_i32(w, value.ref);

	ser_write_end(w);
}

struct pinb_spawner
pinb_spawner_read(struct ser_reader *r, struct ser_value obj)
{
	struct pinb_spawner res = {0};
	struct ser_value key, value;
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("ref"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.ref = value.i32;
		} else if(str8_match(key.str, str8_lit("offset"), 0)) {
			res.offset = ser_read_v2_i32(r, value);
		}
	}
	return res;
}

void
pinb_sfx_sequences_write(struct ser_writer *w, struct pinb_sfx_sequences value)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("len"));
	ser_write_i32(w, value.len);

	ser_write_string(w, str8_lit("items"));
	ser_write_array(w);
	for(usize i = 0; i < value.len; ++i) {
		pinb_sfx_sequence_write(w, value.items[i]);
	}
	ser_write_end(w);

	ser_write_end(w);
}

void
pinb_sfx_sequence_write(struct ser_writer *w, struct pinb_sfx_sequence value)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("type"));
	ser_write_i32(w, value.type);
	ser_write_string(w, str8_lit("reset_time"));
	ser_write_f32(w, value.reset_time);
	ser_write_string(w, str8_lit("vol_min"));
	ser_write_f32(w, value.vol_min);
	ser_write_string(w, str8_lit("vol_max"));
	ser_write_f32(w, value.vol_max);
	ser_write_string(w, str8_lit("pitch_min"));
	ser_write_f32(w, value.pitch_min);
	ser_write_string(w, str8_lit("pitch_max"));
	ser_write_f32(w, value.pitch_max);
	ser_write_string(w, str8_lit("clips_len"));
	ser_write_i32(w, value.clips_len);

	ser_write_string(w, str8_lit("clips"));
	ser_write_array(w);
	for(usize i = 0; i < value.clips_len; ++i) {
		ser_write_string(w, value.clips[i]);
	}
	ser_write_end(w);

	ser_write_end(w);
}

void
pinb_messages_write(struct ser_writer *w, struct pinb_messages value)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("len"));
	ser_write_i32(w, value.len);

	ser_write_string(w, str8_lit("items"));
	ser_write_array(w);
	for(usize i = 0; i < value.len; ++i) {
		pinb_message_write(w, value.items[i]);
	}
	ser_write_end(w);

	ser_write_end(w);
}

void
pinb_message_write(struct ser_writer *w, struct pinb_message value)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("sequence_type"));
	ser_write_i32(w, value.sequence_type);
	ser_write_string(w, str8_lit("sequence_reset_time"));
	ser_write_f32(w, value.sequence_reset_time);
	ser_write_string(w, str8_lit("hide_time"));
	ser_write_f32(w, value.hide_time);
	ser_write_string(w, str8_lit("text_len"));
	ser_write_i32(w, value.text_len);

	ser_write_string(w, str8_lit("text"));
	ser_write_array(w);
	for(usize i = 0; i < value.text_len; ++i) {
		ser_write_string(w, value.text[i]);
	}
	ser_write_end(w);

	ser_write_end(w);
}

void
pinb_actions_write(struct ser_writer *w, struct pinb_actions value)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("len"));
	ser_write_i32(w, value.len);

	ser_write_string(w, str8_lit("items"));
	ser_write_array(w);
	for(usize i = 0; i < value.len; ++i) {
		pinb_action_write(w, value.items[i]);
	}
	ser_write_end(w);

	ser_write_end(w);
}

void
pinb_action_write(struct ser_writer *w, struct pinb_action value)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("action_type"));
	ser_write_i32(w, value.action_type);
	ser_write_string(w, str8_lit("action_ref"));
	ser_write_i32(w, value.action_ref);
	ser_write_string(w, str8_lit("action_arg"));
	ser_write_i32(w, value.action_arg);
	ser_write_string(w, str8_lit("action_delay"));
	ser_write_f32(w, value.action_delay);
	ser_write_string(w, str8_lit("action_cooldown"));
	ser_write_f32(w, value.action_cooldown);
	ser_write_string(w, str8_lit("event_type"));
	ser_write_i32(w, value.event_type);
	ser_write_string(w, str8_lit("event_condition_type"));
	ser_write_i32(w, value.event_condition_type);
	ser_write_string(w, str8_lit("event_condition"));
	ser_write_i32(w, value.event_condition);
	ser_write_string(w, str8_lit("debug"));
	ser_write_i32(w, value.debug);

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
	ser_write_string(w, str8_lit("bg_tex_path"));
	ser_write_string(w, props.bg_tex_path);
	ser_write_end(w);
}

i32
pinb_write(struct ser_writer *w, struct pinb_table pinb)
{
	i32 res = 0;
	ser_write_object(w);

	dbg_assert(pinb.version == 1);
	dbg_assert(pinb.entities_count == arr_len(pinb.entities));
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

struct pinb_reactive_impulse
pinb_reactive_impulse_read(struct ser_reader *r, struct ser_value obj)
{
	struct pinb_reactive_impulse res = {0};
	struct ser_value key, value;
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("magnitude"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.magnitude = value.f32;
		} else if(str8_match(key.str, str8_lit("normalize"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.normalize = value.i32;
		}
	}
	return res;
}

struct pinb_force_field
pinb_force_field_read(struct ser_reader *r, struct ser_value obj)
{
	struct pinb_force_field res = {0};
	struct ser_value key, value;
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("magnitude"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.magnitude = value.f32;
		} else if(str8_match(key.str, str8_lit("angle"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.angle = value.f32;
		} else if(str8_match(key.str, str8_lit("is_enabled"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.is_enabled = value.i32;
		}
	}
	return res;
}

struct pinb_attractor
pinb_attractor_read(struct ser_reader *r, struct ser_value obj)
{
	struct pinb_attractor res = {0};
	struct ser_value key, value;
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("flags"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.flags = value.i32;
		} else if(str8_match(key.str, str8_lit("offset"), 0)) {
			res.offset = ser_read_v2(r, value);
		} else if(str8_match(key.str, str8_lit("radius"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.radius = value.f32;
		} else if(str8_match(key.str, str8_lit("force"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.force = value.f32;
		} else if(str8_match(key.str, str8_lit("damping"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.damping = value.f32;
		} else if(str8_match(key.str, str8_lit("distance_threshold"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.distance_threshold = value.f32;
		}
	}
	return res;
}

struct pinb_reactive_sprite_offset
pinb_reactive_sprite_offset_read(struct ser_reader *r, struct ser_value obj)
{
	struct pinb_reactive_sprite_offset res = {0};
	struct ser_value key, value;
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("magnitude"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.magnitude = value.f32;
		} else if(str8_match(key.str, str8_lit("delay"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.delay = value.f32;
		} else if(str8_match(key.str, str8_lit("ref"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.ref = value.i32;
		}
	}
	return res;
}

struct pinb_reactive_animation
pinb_reactive_animation_read(struct ser_reader *r, struct ser_value obj)
{
	struct pinb_reactive_animation res = {0};
	struct ser_value key, value;
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("animation_index"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.animation_index = value.i32;
		}
	}
	return res;
}

struct pinb_charged_impulse
pinb_charged_impulse_read(struct ser_reader *r, struct ser_value obj)
{
	struct pinb_charged_impulse res = {0};
	struct ser_value key, value;
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("angle"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.angle = value.f32;
		} else if(str8_match(key.str, str8_lit("magnitude"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.magnitude = value.f32;
		} else if(str8_match(key.str, str8_lit("charge_speed"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.charge_speed = value.f32;
		} else if(str8_match(key.str, str8_lit("release_speed"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.release_speed = value.f32;
		} else if(str8_match(key.str, str8_lit("reset_target"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.reset_target = value.i32;
		} else if(str8_match(key.str, str8_lit("auto_shoot"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.auto_shoot = value.i32;
		}
	}
	return res;
}

struct pinb_plunger
pinb_plunger_read(struct ser_reader *r, struct ser_value obj)
{
	struct pinb_plunger res = {0};
	struct ser_value key, value;
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("charge_force_min"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.charge_force_min = value.f32;
		} else if(str8_match(key.str, str8_lit("charge_force_max"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.charge_force_max = value.f32;
		} else if(str8_match(key.str, str8_lit("release_force_min"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.release_force_min = value.f32;
		} else if(str8_match(key.str, str8_lit("release_force_max"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.release_force_max = value.f32;
		}
	}

	return res;
}

struct pinb_spinner
pinb_spinner_read(struct ser_reader *r, struct ser_value obj)
{
	struct pinb_spinner res = {0};
	struct ser_value key, value;
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("damping"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.damping = value.f32;
		} else if(str8_match(key.str, str8_lit("spin_force"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.spin_force = value.f32;
		} else if(str8_match(key.str, str8_lit("stop_threshold"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.stop_threshold = value.f32;
		}
	}
	return res;
}

struct pinb_bucket
pinb_bucket_read(struct ser_reader *r, struct ser_value obj)
{
	struct pinb_bucket res = {0};
	struct ser_value key, value;
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("animation_shoot"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.animation_shoot = value.i32;
		} else if(str8_match(key.str, str8_lit("animation_on"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.animation_on = value.i32;
		} else if(str8_match(key.str, str8_lit("animation_off"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.animation_off = value.i32;
		} else if(str8_match(key.str, str8_lit("impulse_angle"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.impulse_angle = value.f32;
		} else if(str8_match(key.str, str8_lit("impulse_magnitude"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.impulse_magnitude = value.f32;
		} else if(str8_match(key.str, str8_lit("delay"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.delay = value.f32;
		}
	}
	return res;
}

struct pinb_ball_saver
pinb_ball_saver_read(struct ser_reader *r, struct ser_value obj)
{
	struct pinb_ball_saver res = {0};
	struct ser_value key, value;
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("is_enabled"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.is_enabled = value.i32;
		} else if(str8_match(key.str, str8_lit("duration"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.duration = value.f32;
		} else if(str8_match(key.str, str8_lit("save_delay"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.save_delay = value.f32;
		}
	}
	return res;
}

struct pinb_flipper
pinb_flipper_read(struct ser_reader *r, struct ser_value obj)
{
	struct pinb_flipper res = {0};
	struct ser_value key, value;
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("velocity_easing_function"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.velocity_easing_function = value.i32;
		} else if(str8_match(key.str, str8_lit("velocity_scale"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.velocity_scale = value.f32;
		} else if(str8_match(key.str, str8_lit("velocity_radius_min"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.velocity_radius_min = value.f32;
		} else if(str8_match(key.str, str8_lit("velocity_radius_max"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.velocity_radius_max = value.f32;
		}
	}
	return res;
}

struct pinb_flip
pinb_flip_read(struct ser_reader *r, struct ser_value obj)
{
	struct pinb_flip res = {0};
	struct ser_value key, value;
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("type"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.type = value.i32;
		} else if(str8_match(key.str, str8_lit("is_enabled"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.is_enabled = value.i32;
		}
	}
	return res;
}

struct pinb_gravity
pinb_gravity_read(struct ser_reader *r, struct ser_value obj)
{
	struct pinb_gravity res = {0};
	struct ser_value key, value;
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("value"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.value = value.f32;
		}
	}
	return res;
}

struct pinb_counter
pinb_counter_read(struct ser_reader *r, struct ser_value obj)
{
	struct pinb_counter res = {0};
	struct ser_value key, value;
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("min"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.min = value.i32;
		} else if(str8_match(key.str, str8_lit("max"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.max = value.i32;
		} else if(str8_match(key.str, str8_lit("value"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.value = value.i32;
		} else if(str8_match(key.str, str8_lit("resolution"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.resolution = value.i32;
		}
	}
	return res;
}

struct pinb_collision_layer
pinb_collision_layer_read(struct ser_reader *r, struct ser_value obj)
{
	struct pinb_collision_layer res = {0};
	struct ser_value key, value;
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("layer"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.layer = value.i32;
		}
	}

	return res;
}

struct pinb_crank_animation
pinb_crank_animation_read(struct ser_reader *r, struct ser_value obj)
{
	struct pinb_crank_animation res = {0};
	struct ser_value key, value;
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("interval"), 0)) {
			dbg_assert(value.type == SER_TYPE_F32);
			res.interval = value.f32;
		}
	}
	return res;
}

struct pinb_reset
pinb_reset_read(struct ser_reader *r, struct ser_value obj)
{
	struct pinb_reset res = {0};
	struct ser_value key, value;
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("flags"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.flags = value.i32;
		}
	}
	return res;
}

struct pinb_spr
pinb_spr_read(struct ser_reader *r, struct ser_value obj)
{
	struct pinb_spr res = {0};
	struct ser_value key, value;
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("path"), 0)) {
			dbg_assert(value.type == SER_TYPE_STRING);
			res.path = value.str;
		} else if(str8_match(key.str, str8_lit("flip"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.flip = value.i32;
		} else if(str8_match(key.str, str8_lit("layer"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.layer = value.i32;
		} else if(str8_match(key.str, str8_lit("y_sort"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.y_sort = value.i32;
		} else if(str8_match(key.str, str8_lit("y_sort_offset"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.y_sort_offset = value.i32;
		} else if(str8_match(key.str, str8_lit("offset"), 0)) {
			dbg_assert(value.type == SER_TYPE_ARRAY);
			res.offset = ser_read_v2(r, value);
		}
	}

	return res;
}

struct pinb_sfx_sequences
pinb_sfx_sequences_read(struct ser_reader *r, struct ser_value obj, struct alloc alloc)
{
	struct pinb_sfx_sequences res = {0};
	struct ser_value key, value;
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("len"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.len   = value.i32;
			res.items = arr_new(res.items, value.i32, alloc);
		} else if(str8_match(key.str, str8_lit("items"), 0)) {
			struct ser_value item_value;
			while(ser_iter_array(r, value, &item_value)) {
				arr_push(res.items, pinb_sfx_sequence_read(r, item_value, alloc));
			}
			dbg_assert(res.len == arr_len(res.items));
		}
	}
	return res;
}

struct pinb_messages
pinb_messages_read(struct ser_reader *r, struct ser_value obj, struct alloc alloc)
{
	struct pinb_messages res = {0};
	struct ser_value key, value;
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("len"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.len   = value.i32;
			res.items = arr_new(res.items, value.i32, alloc);
		} else if(str8_match(key.str, str8_lit("items"), 0)) {
			struct ser_value sequence_value;
			while(ser_iter_array(r, value, &sequence_value)) {
				arr_push(res.items, pinb_message_read(r, sequence_value, alloc));
			}
			dbg_assert(res.len == arr_len(res.items));
		}
	}
	return res;
}

struct pinb_actions
pinb_actions_read(struct ser_reader *r, struct ser_value obj, struct alloc alloc)
{
	struct pinb_actions res = {0};
	struct ser_value key, value;
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("len"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.len   = value.i32;
			res.items = arr_new(res.items, value.i32, alloc);
		} else if(str8_match(key.str, str8_lit("items"), 0)) {
			struct ser_value item_value;
			while(ser_iter_array(r, value, &item_value)) {
				arr_push(res.items, pinb_action_read(r, item_value));
			}
			dbg_assert(res.len == arr_len(res.items));
		}
	}
	return res;
}

struct pinb_physics_props
pinb_physics_props_read(struct ser_reader *r, struct ser_value obj)
{
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	struct pinb_physics_props res = {0};
	struct ser_value key, value;
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
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
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	struct pinb_flippers_props res = {0};
	struct ser_value key, value;
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
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
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	struct pinb_table_props res = {0};
	struct ser_value key, value;
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("physics_props"), 0)) {
			res.physics_props = pinb_physics_props_read(r, value);
		} else if(str8_match(key.str, str8_lit("flippers_props"), 0)) {
			res.flippers_props = pinb_flippers_props_read(r, value);
		} else if(str8_match(key.str, str8_lit("bg_tex_path"), 0)) {
			dbg_assert(value.type == SER_TYPE_STRING);
			res.bg_tex_path = value.str;
		}
	}

	return res;
}

struct pinb_animator_transition
pinb_animator_transition_read(struct ser_reader *r, struct ser_value arr)
{
	struct pinb_animator_transition res = {0};
	struct ser_value value;
	dbg_assert(arr.type == SER_TYPE_ARRAY);
	ser_iter_array(r, arr, &value);
	dbg_assert(value.type == SER_TYPE_I32);
	res.from = value.i32;
	ser_iter_array(r, arr, &value);
	dbg_assert(value.type == SER_TYPE_I32);
	res.to = value.i32;
	return res;
}

struct pinb_animator_transitions
pinb_animator_transitions_read(struct ser_reader *r, struct ser_value obj, struct alloc alloc)
{
	struct pinb_animator_transitions res = {0};
	struct ser_value key, value;
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	size len = 0;
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("len"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			len       = value.i32;
			res.items = arr_new(res.items, len, alloc);
		} else if(str8_match(key.str, str8_lit("items"), 0)) {
			dbg_assert(value.type == SER_TYPE_ARRAY);
			struct ser_value item_value = {0};
			while(ser_iter_array(r, value, &item_value)) {
				res.items[res.len++] = pinb_animator_transition_read(r, item_value);
			}
		}
	}
	dbg_assert(len == res.len);
	return res;
}

struct pinb_animator
pinb_animator_read(struct ser_reader *r, struct ser_value obj, struct alloc alloc)
{
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	struct pinb_animator res = {0};
	struct ser_value key, value;

	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("play_on_start"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.play_on_start = value.i32;
		} else if(str8_match(key.str, str8_lit("initial_animation"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.initial_animation = value.i32;
		} else if(str8_match(key.str, str8_lit("transitions"), 0)) {
			dbg_assert(value.type == SER_TYPE_OBJECT);
			res.transitions = pinb_animator_transitions_read(r, value, alloc);
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
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("is_enabled"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.is_enabled = value.i32;
		} else if(str8_match(key.str, str8_lit("shapes"), 0)) {
			res.shapes = col_shapes_read(r, value);
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
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("is_enabled"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.is_enabled = value.i32;
		} else if(str8_match(key.str, str8_lit("value"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.value = value.i32;
		} else if(str8_match(key.str, str8_lit("animation_on"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.animation_on = value.i32;
		} else if(str8_match(key.str, str8_lit("animation_off"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
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
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("prev"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.prev = value.i32;
		} else if(str8_match(key.str, str8_lit("next"), 0)) {
			dbg_assert(value.type == SER_TYPE_I32);
			res.next = value.i32;
		}
	}

	return res;
}

struct pinb_sfx_sequence
pinb_sfx_sequence_read(struct ser_reader *r, struct ser_value obj, struct alloc alloc)
{
	struct pinb_sfx_sequence res = {0};
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	struct ser_value key, value;
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
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
			res.clips     = arr_new(res.clips, res.clips_len, alloc);
		} else if(str8_match(key.str, str8_lit("clips"), 0)) {
			dbg_assert(value.type == SER_TYPE_ARRAY);
			struct ser_value clip_value;
			usize i = 0;
			while(ser_iter_array(r, value, &clip_value)) {
				res.clips[i++] = clip_value.str;
			}
		}
	}
	return res;
}

struct pinb_message
pinb_message_read(struct ser_reader *r, struct ser_value obj, struct alloc alloc)
{
	struct pinb_message res = {0};
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	struct ser_value key, value;
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("sequence_type"), 0)) {
			res.sequence_type = value.i32;
		} else if(str8_match(key.str, str8_lit("sequence_reset_time"), 0)) {
			res.sequence_reset_time = value.f32;
		} else if(str8_match(key.str, str8_lit("hide_time"), 0)) {
			res.hide_time = value.f32;
		} else if(str8_match(key.str, str8_lit("text_len"), 0)) {
			res.text_len = value.i32;
			res.text     = arr_new(res.text, res.text_len, alloc);
		} else if(str8_match(key.str, str8_lit("text"), 0)) {
			dbg_assert(value.type == SER_TYPE_ARRAY);
			struct ser_value clip_value;
			usize i = 0;
			while(ser_iter_array(r, value, &clip_value)) {
				res.text[i++] = clip_value.str;
			}
		}
	}
	return res;
}

struct pinb_action
pinb_action_read(struct ser_reader *r, struct ser_value obj)
{
	struct pinb_action res = {0};
	dbg_assert(obj.type == SER_TYPE_OBJECT);
	struct ser_value key, value;
	while(ser_iter_object(r, obj, &key, &value)) {
		dbg_assert(key.type == SER_TYPE_STRING);
		if(str8_match(key.str, str8_lit("action_type"), 0)) {
			res.action_type = value.i32;
		} else if(str8_match(key.str, str8_lit("action_ref"), 0)) {
			res.action_ref = value.i32;
		} else if(str8_match(key.str, str8_lit("action_arg"), 0)) {
			res.action_arg = value.i32;
		} else if(str8_match(key.str, str8_lit("action_delay"), 0)) {
			res.action_delay = value.f32;
		} else if(str8_match(key.str, str8_lit("action_cooldown"), 0)) {
			res.action_cooldown = value.f32;
		} else if(str8_match(key.str, str8_lit("event_type"), 0)) {
			res.event_type = value.i32;
		} else if(str8_match(key.str, str8_lit("event_condition_type"), 0)) {
			res.event_condition_type = value.i32;
		} else if(str8_match(key.str, str8_lit("event_condition"), 0)) {
			res.event_condition = value.i32;
		} else if(str8_match(key.str, str8_lit("debug"), 0)) {
			res.debug = value.i32;
		}
	}
	return res;
}

void
pinb_entity_spr_write(struct ser_writer *w, struct pinb_entity entity)
{
	ser_write_object(w);

	ser_write_string(w, str8_lit("path"));
	ser_write_string(w, entity.spr.path);
	ser_write_string(w, str8_lit("flip"));
	ser_write_i32(w, entity.spr.flip);
	ser_write_string(w, str8_lit("layer"));
	ser_write_i32(w, entity.spr.layer);
	ser_write_string(w, str8_lit("y_sort"));
	ser_write_i32(w, entity.spr.y_sort);
	ser_write_string(w, str8_lit("y_sort_offset"));
	ser_write_i32(w, entity.spr.y_sort_offset);

	ser_write_string(w, str8_lit("offset"));
	ser_write_v2(w, entity.spr.offset);

	ser_write_end(w);
}
