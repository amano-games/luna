#include "pinbtjson.h"
#include "arr.h"
#include "json.h"
#include "mathfunc.h"
#include "mem-arena.h"
#include "path.h"
#include "pinb/pinb-ser.h"
#include "poly.h"
#include "serialize/serialize.h"
#include "str.h"
#include "sys-io.h"
#include "sys-log.h"
#include "sys.h"
#include "tools/png/png.h"
#include "tools/wav/wav.h"
#include "dbg.h"

struct pinbtjson_res
pinbtjson_handle_flip(str8 json, jsmntok_t *tokens, i32 index)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);

	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		if(json_eq(json, key, str8_lit("type")) == 0) {
			res.flip.type = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("is_enabled")) == 0) {
			res.flip.is_enabled = json_parse_bool32(json, value);
		}
	}

	return res;
}

struct pinbtjson_res
pinbtjson_handle_reactive_impulse(str8 json, jsmntok_t *tokens, i32 index)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);

	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		if(json_eq(json, key, str8_lit("magnitude")) == 0) {
			res.reactive_impulse.magnitude = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("normalize")) == 0) {
			res.reactive_impulse.normalize = json_parse_bool32(json, value);
		}
	}

	return res;
}

struct pinbtjson_res
pinbtjson_handle_force_field(str8 json, jsmntok_t *tokens, i32 index)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);

	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		if(json_eq(json, key, str8_lit("magnitude")) == 0) {
			res.force_field.magnitude = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("angle_degrees")) == 0) {
			res.force_field.angle = json_parse_f32(json, value) * DEG_TO_TURN;
		} else if(json_eq(json, key, str8_lit("is_enabled")) == 0) {
			res.force_field.is_enabled = json_parse_bool32(json, value);
		}
	}

	return res;
}

struct pinbtjson_res
pinbtjson_handle_attractor(str8 json, jsmntok_t *tokens, i32 index)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);

	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		if(json_eq(json, key, str8_lit("flags")) == 0) {
			res.attractor.flags = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("offset")) == 0) {
			dbg_assert(value->type == JSMN_ARRAY);
			res.attractor.offset.x = json_parse_f32(json, tokens + i + 2);
			res.attractor.offset.y = json_parse_f32(json, tokens + i + 3);
		} else if(json_eq(json, key, str8_lit("radius")) == 0) {
			res.attractor.radius = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("force")) == 0) {
			res.attractor.force = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("damping")) == 0) {
			res.attractor.damping = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("distance_threshold")) == 0) {
			res.attractor.distance_threshold = json_parse_f32(json, value);
		}
	}

	return res;
}

struct pinbtjson_res
pinbtjson_handle_reactive_animation(str8 json, jsmntok_t *tokens, i32 index)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);

	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		if(json_eq(json, key, str8_lit("animation_index")) == 0) {
			res.reactive_animation.animation_index = json_parse_i32(json, value);
		}
	}

	return res;
}

struct pinbtjson_res
pinbtjson_handle_reactive_sprite_offset(str8 json, jsmntok_t *tokens, i32 index)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);

	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		if(json_eq(json, key, str8_lit("delay")) == 0) {
			res.reactive_sprite_offset.delay = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("magnitude")) == 0) {
			res.reactive_sprite_offset.magnitude = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("ref")) == 0) {
			res.reactive_sprite_offset.ref = json_parse_i32(json, value);
		}
	}

	return res;
}

struct pinbtjson_res
pinbtjson_handle_animator_transition(str8 json, jsmntok_t *tokens, i32 index)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_ARRAY);
	dbg_assert(root->size == 2);

	jsmntok_t *from              = tokens + index + 1;
	jsmntok_t *to                = tokens + index + 2;
	res.animator_transition.from = json_parse_i32(json, from);
	res.animator_transition.to   = json_parse_i32(json, to);
	res.token_count              = root->size + 1;

	return res;
}

struct pinbtjson_res
pinbtjson_handle_animator(str8 json, jsmntok_t *tokens, i32 index, struct alloc alloc)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);

	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		if(json_eq(json, key, str8_lit("play_on_start")) == 0) {
			res.animator.play_on_start = json_parse_bool32(json, value);
		} else if(json_eq(json, key, str8_lit("initial_animation")) == 0) {
			res.animator.initial_animation = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("transitions")) == 0) {
			dbg_assert(value->type == JSMN_ARRAY);
			res.animator.transitions.len   = value->size;
			res.animator.transitions.items = arr_new(res.animator.transitions.items, value->size, alloc);
			for(usize j = 0; j < (usize)value->size; ++j) {
				i32 item_index  = i + 2;
				jsmntok_t *item = tokens + item_index;
				dbg_assert(item->type == JSMN_ARRAY);
				struct pinbtjson_res item_res     = pinbtjson_handle_animator_transition(json, tokens, item_index);
				res.animator.transitions.items[j] = item_res.animator_transition;
				i += item_res.token_count;
			}
		}
	}

	return res;
}

struct pinbtjson_res
pinbtjson_handle_gravity(str8 json, jsmntok_t *tokens, i32 index)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);

	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		if(json_eq(json, key, str8_lit("value")) == 0) {
			res.gravity.value = json_parse_f32(json, value);
		}
	}

	return res;
}

struct pinbtjson_res
pinbtjson_handle_counter(str8 json, jsmntok_t *tokens, i32 index)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);

	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		if(json_eq(json, key, str8_lit("min")) == 0) {
			res.counter.min = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("max")) == 0) {
			res.counter.max = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("value")) == 0) {
			res.counter.value = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("resolution")) == 0) {
			res.counter.resolution = json_parse_i32(json, value);
		}
	}

	return res;
}

struct pinbtjson_res
pinbtjson_handle_collision_layer(str8 json, jsmntok_t *tokens, i32 index)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);

	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		if(json_eq(json, key, str8_lit("layer")) == 0) {
			res.collision_layer.layer = json_parse_i32(json, value);
		}
	}

	return res;
}

struct pinbtjson_res
pinbtjson_handle_crank_animation(str8 json, jsmntok_t *tokens, i32 index)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);

	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		if(json_eq(json, key, str8_lit("interval")) == 0) {
			res.crank_animation.interval = json_parse_f32(json, value);
		}
	}

	return res;
}

struct pinbtjson_res
pinbtjson_handle_reset(str8 json, jsmntok_t *tokens, i32 index)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);

	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		if(json_eq(json, key, str8_lit("flags")) == 0) {
			res.reset.flags = json_parse_i32(json, value);
		}
	}

	return res;
}

struct pinbtjson_res
pinbtjson_handle_charged_impulse(str8 json, jsmntok_t *tokens, i32 index)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);

	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		if(json_eq(json, key, str8_lit("angle_degrees")) == 0) {
			res.charged_impulse.angle = json_parse_f32(json, value) * DEG_TO_TURN;
		} else if(json_eq(json, key, str8_lit("magnitude")) == 0) {
			res.charged_impulse.magnitude = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("charge_speed")) == 0) {
			res.charged_impulse.charge_speed = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("release_speed")) == 0) {
			res.charged_impulse.release_speed = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("reset_target")) == 0) {
			res.charged_impulse.reset_target = json_parse_bool32(json, value);
		} else if(json_eq(json, key, str8_lit("auto_shoot")) == 0) {
			res.charged_impulse.auto_shoot = json_parse_bool32(json, value);
		} else if(json_eq(json, key, str8_lit("auto_shoot_hold")) == 0) {
			res.charged_impulse.auto_shoot_hold = json_parse_f32(json, value);
		}
	}

	return res;
}

struct pinbtjson_res
pinbtjson_handle_plunger(str8 json, jsmntok_t *tokens, i32 index)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);

	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		if(json_eq(json, key, str8_lit("charge_force_max")) == 0) {
			res.plunger.charge_force_max = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("charge_force_min")) == 0) {
			res.plunger.charge_force_min = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("release_force_max")) == 0) {
			res.plunger.release_force_max = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("release_force_min")) == 0) {
			res.plunger.release_force_min = json_parse_f32(json, value);
		}
	}

	return res;
}

struct pinbtjson_res
pinbtjson_handle_spinner(str8 json, jsmntok_t *tokens, i32 index)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);

	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		if(json_eq(json, key, str8_lit("damping")) == 0) {
			res.spinner.damping = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("spin_force")) == 0) {
			res.spinner.spin_force = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("stop_threshold")) == 0) {
			res.spinner.stop_threshold = json_parse_f32(json, value);
		}
	}

	return res;
}

struct pinbtjson_res
pinbtjson_handle_bucket(str8 json, jsmntok_t *tokens, i32 index)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);

	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		if(json_eq(json, key, str8_lit("animation_shoot")) == 0) {
			res.bucket.animation_shoot = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("animation_on")) == 0) {
			res.bucket.animation_on = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("animation_off")) == 0) {
			res.bucket.animation_off = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("delay")) == 0) {
			res.bucket.delay = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("impulse_angle_degrees")) == 0) {
			res.bucket.impulse_angle = json_parse_f32(json, value) * DEG_TO_RAD;
		} else if(json_eq(json, key, str8_lit("impulse_magnitude")) == 0) {
			res.bucket.impulse_magnitude = json_parse_f32(json, value);
		}
	}

	return res;
}

struct pinbtjson_res
pinbtjson_handle_ball_saver(str8 json, jsmntok_t *tokens, i32 index)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);

	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		if(json_eq(json, key, str8_lit("is_enabled")) == 0) {
			res.ball_saver.is_enabled = json_parse_bool32(json, value);
		} else if(json_eq(json, key, str8_lit("duration")) == 0) {
			res.ball_saver.duration = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("save_delay")) == 0) {
			res.ball_saver.save_delay = json_parse_f32(json, value);
		}
	}

	return res;
}

struct pinbtjson_res
pinbtjson_handle_flipper(str8 json, jsmntok_t *tokens, i32 index)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);

	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		if(json_eq(json, key, str8_lit("velocity_easing_function")) == 0) {
			res.flipper.velocity_easing_function = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("velocity_radius_max")) == 0) {
			res.flipper.velocity_radius_max = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("velocity_radius_min")) == 0) {
			res.flipper.velocity_radius_min = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("velocity_scale")) == 0) {
			res.flipper.velocity_scale = json_parse_f32(json, value);
		}
	}

	return res;
}

struct pinbtjson_res
pinbtjson_handle_col_cir(str8 json, jsmntok_t *tokens, i32 index)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
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

struct pinbtjson_res
pinbtjson_handle_col_aabb(str8 json, jsmntok_t *tokens, i32 index)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_ARRAY);
	dbg_assert(root->size == 4);
	res.token_count = 5;

	i32 i          = index;
	res.aabb.min.x = json_parse_f32(json, tokens + index + 1);
	res.aabb.min.y = json_parse_f32(json, tokens + index + 2);
	res.aabb.max.x = json_parse_f32(json, tokens + index + 3);
	res.aabb.max.y = json_parse_f32(json, tokens + index + 4);

	return res;
}

struct pinbtjson_res
pinbtjson_handle_sfx_sequence(str8 json, jsmntok_t *tokens, i32 index, struct alloc alloc)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);
	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		str8 key_str     = json_str8(json, key);
		str8 value_str   = json_str8(json, value);
		if(json_eq(json, key, str8_lit("type")) == 0) {
			res.sfx_sequence.type = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("vol_min")) == 0) {
			res.sfx_sequence.vol_min = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("vol_max")) == 0) {
			res.sfx_sequence.vol_max = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("pitch_min")) == 0) {
			res.sfx_sequence.pitch_min = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("pitch_max")) == 0) {
			res.sfx_sequence.pitch_max = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("reset_time")) == 0) {
			res.sfx_sequence.reset_time = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("clips")) == 0) {
			dbg_assert(value->type == JSMN_ARRAY);
			res.sfx_sequence.clips = arr_new(res.sfx_sequence.clips, value->size, alloc);
			for(usize j = 0; j < (usize)value->size; ++j) {
				i32 item_index  = i + j + 2;
				jsmntok_t *item = tokens + item_index;
				dbg_assert(item->type == JSMN_STRING);
				str8 wav_path                                        = json_str8(json, item);
				str8 snd_path                                        = make_file_name_with_ext(alloc, wav_path, str8_lit(SND_FILE_EXT));
				res.sfx_sequence.clips[res.sfx_sequence.clips_len++] = snd_path;
			}
			i += value->size;
		}
	}
	return res;
}

struct pinbtjson_res
pinbtjson_handle_message(str8 json, jsmntok_t *tokens, i32 index, struct alloc alloc)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);
	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		str8 key_str     = json_str8(json, key);
		str8 value_str   = json_str8(json, value);
		if(json_eq(json, key, str8_lit("sequence_type")) == 0) {
			res.message.sequence_type = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("sequence_reset_time")) == 0) {
			res.message.sequence_reset_time = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("hide_time")) == 0) {
			res.message.hide_time = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("text")) == 0) {
			dbg_assert(value->type == JSMN_ARRAY);
			res.message.text = arr_new(res.message.text, value->size, alloc);
			for(usize j = 0; j < (usize)value->size; ++j) {
				i32 item_index  = i + j + 2;
				jsmntok_t *item = tokens + item_index;
				dbg_assert(item->type == JSMN_STRING);
				res.message.text[res.message.text_len++] = json_str8_cpy_push(json, item, alloc);
			}
			i += value->size;
		}
	}
	return res;
}

struct pinbtjson_res
pinbtjson_handle_action(str8 json, jsmntok_t *tokens, i32 index, struct alloc alloc)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);
	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		str8 key_str     = json_str8(json, key);
		str8 value_str   = json_str8(json, value);
		if(json_eq(json, key, str8_lit("action_type")) == 0) {
			res.action.action_type = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("action_ref")) == 0) {
			res.action.action_ref = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("action_argument")) == 0) {
			res.action.action_arg = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("action_delay")) == 0) {
			res.action.action_delay = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("action_cooldown")) == 0) {
			res.action.action_cooldown = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("event_type")) == 0) {
			res.action.event_type = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("event_condition_type")) == 0) {
			res.action.event_condition_type = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("event_condition")) == 0) {
			res.action.event_condition = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("debug")) == 0) {
			res.action.debug = json_parse_bool32(json, value);
		}
	}
	return res;
}

struct pinbtjson_res
pinbtjson_handle_spr(str8 json, jsmntok_t *tokens, i32 index, struct alloc alloc)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);

	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		str8 key_str     = json_str8(json, key);
		str8 value_str   = json_str8(json, value);
		if(json_eq(json, key, str8_lit("path")) == 0) {
			str8 path    = json_str8(json, value);
			res.spr.path = make_file_name_with_ext(alloc, path, str8_lit(TEX_EXT));
		} else if(json_eq(json, key, str8_lit("flip")) == 0) {
			res.spr.flip = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("layer")) == 0) {
			res.spr.layer = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("y_sort")) == 0) {
			res.spr.y_sort = json_parse_bool32(json, value);
		} else if(json_eq(json, key, str8_lit("y_sort_offset")) == 0) {
			res.spr.y_sort_offset = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("offset")) == 0) {
			dbg_assert(value->type == JSMN_ARRAY);
			res.spr.offset.x = json_parse_f32(json, tokens + i + 2);
			res.spr.offset.y = json_parse_f32(json, tokens + i + 3);
		}
	}

	return res;
}

struct pinbtjson_res
pinbtjson_handle_col_shape(str8 json, jsmntok_t *tokens, i32 index, struct alloc alloc, struct alloc scratch)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);
	for(usize i = index + 1; i < index + res.token_count; i++) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		if(json_eq(json, key, str8_lit("poly")) == 0) {
			dbg_assert(value->type == JSMN_ARRAY);
			i++;
			size vert_count  = value->size / 2;
			struct v2 *verts = arr_new(verts, vert_count, scratch);
			for(size j = 0; j < vert_count; ++j) {
				verts[j].x = json_parse_f32(json, tokens + ++i);
				verts[j].y = json_parse_f32(json, tokens + ++i);
			}
			size vert_count_n = poly_remove_collinear_points(verts, vert_count, 0);
			if(vert_count_n != vert_count) {
				log_info("pinb-gen", "Removed points from polygon from %d -> %d", (int)vert_count, (int)vert_count_n);
			}
			vert_count       = vert_count_n;
			bool32 is_convex = poly_is_convex(verts, vert_count);
			struct mesh mesh = {0};
			if(is_convex) {
				if(vert_count > POLY_MAX_VERTS) {
					log_error("pinb-gen", "Polygon has too many verts %d", (int)vert_count);
				}
				dbg_assert(vert_count <= POLY_MAX_VERTS);
				mesh.count          = 1;
				mesh.items          = alloc.allocf(alloc.ctx, sizeof(*mesh.items));
				mesh.items[0].count = vert_count;
				for(size j = 0; j < vert_count; ++j) {
					mesh.items[0].verts[j] = verts[j];
				}
			} else {
				mesh = poly_decomp(verts, vert_count, scratch, scratch);
				dbg_assert(mesh.count <= (size)ARRLEN(res.col_shapes.items));
			}

			res.col_shapes.count = mesh.count;
			for(size j = 0; j < mesh.count; ++j) {
				struct poly poly                   = mesh.items[j];
				res.col_shapes.items[j].type       = COL_TYPE_POLY;
				res.col_shapes.items[j].poly.count = poly.count;
				mcpy_array(res.col_shapes.items[j].poly.verts, poly.verts);
				dbg_assert(poly_is_convex(poly.verts, poly.count));
			}

		} else if(json_eq(json, key, str8_lit("aabb")) == 0) {
			i++;
			struct pinbtjson_res item_res = pinbtjson_handle_col_aabb(json, tokens, i);
			res.col_shapes.count          = 1;
			res.col_shapes.items[0].type  = COL_TYPE_AABB;
			res.col_shapes.items[0].aabb  = item_res.aabb;
			i += item_res.token_count;
		} else if(json_eq(json, key, str8_lit("cir")) == 0) {
			i++;
			struct pinbtjson_res item_res = pinbtjson_handle_col_cir(json, tokens, i);
			res.col_shapes.count          = 1;
			res.col_shapes.items[0].type  = COL_TYPE_CIR;
			res.col_shapes.items[0].cir   = item_res.cir;
			i += item_res.token_count;
		} else if(json_eq(json, key, str8_lit("capsule")) == 0) {
			dbg_assert(value->type == JSMN_ARRAY);
			i += 2;
			res.col_shapes.count         = 1;
			res.col_shapes.items[0].type = COL_TYPE_CAPSULE;
			for(i32 j = 0; j < value->size; j++) {
				jsmntok_t *item                         = tokens + i;
				struct pinbtjson_res item_res           = pinbtjson_handle_col_cir(json, tokens, i);
				res.col_shapes.items[0].capsule.cirs[j] = item_res.cir;
				i += item_res.token_count;
			}
		}
	}

	return res;
}

struct pinbtjson_res
pinbtjson_handle_rigid_body(str8 json, jsmntok_t *tokens, i32 index, struct alloc alloc, struct alloc scratch)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);
	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
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
			struct pinbtjson_res item_res = pinbtjson_handle_col_shape(json, tokens, i + 1, alloc, scratch);
			res.body.shapes.count         = item_res.col_shapes.count;
			mcpy_array(res.body.shapes.items, item_res.col_shapes.items);
			i += item_res.token_count - 1;
		}
	}
	body_init(&res.body);
	return res;
}

struct pinbtjson_res
pinbtjson_handle_sensor(str8 json, jsmntok_t *tokens, i32 index, struct alloc alloc, struct alloc scratch)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);
	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		str8 key_str     = json_str8(json, key);
		str8 value_str   = json_str8(json, value);
		if(json_eq(json, key, str8_lit("is_enabled")) == 0) {
			res.sensor.is_enabled = json_parse_bool32(json, value);
		} else if(json_eq(json, key, str8_lit("collision_shape")) == 0) {
			struct pinbtjson_res item_res = pinbtjson_handle_col_shape(json, tokens, i + 1, alloc, scratch);
			res.sensor.shapes.count       = item_res.col_shapes.count;
			mcpy_array(res.sensor.shapes.items, item_res.col_shapes.items);
			i += item_res.token_count - 1;
		}
	}
	return res;
}

struct pinbtjson_res
pinbtjson_handle_switch_value(str8 json, jsmntok_t *tokens, i32 index, struct alloc alloc)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);
	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		str8 key_str     = json_str8(json, key);
		str8 value_str   = json_str8(json, value);
		if(json_eq(json, key, str8_lit("is_enabled")) == 0) {
			res.switch_value.is_enabled = json_parse_bool32(json, value);
		} else if(json_eq(json, key, str8_lit("value")) == 0) {
			res.switch_value.value = json_parse_bool32(json, value);
		} else if(json_eq(json, key, str8_lit("animation_on")) == 0) {
			res.switch_value.animation_on = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("animation_off")) == 0) {
			res.switch_value.animation_off = json_parse_i32(json, value);
		}
	}
	return res;
}

struct pinbtjson_res
pinbtjson_handle_switch_list(str8 json, jsmntok_t *tokens, i32 index, struct alloc alloc)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);
	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		str8 key_str     = json_str8(json, key);
		str8 value_str   = json_str8(json, value);
		if(json_eq(json, key, str8_lit("next")) == 0) {
			res.switch_list.next = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("prev")) == 0) {
			res.switch_list.prev = json_parse_i32(json, value);
		}
	}
	return res;
}

struct pinbtjson_res
pinbtjson_handle_spawner(str8 json, jsmntok_t *tokens, i32 index, struct alloc alloc)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);

	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		if(json_eq(json, key, str8_lit("ref")) == 0) {
			res.spawner.ref = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("type")) == 0) {
			res.spawner.type = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("zones")) == 0) {
			dbg_assert(value->type == JSMN_ARRAY);
			res.spawner.zones = arr_new(res.spawner.zones, value->size, alloc);
			for(usize j = 0; j < (usize)value->size; ++j) {
				i32 item_index  = i + j + 2;
				jsmntok_t *item = tokens + item_index;
				dbg_assert(item->type == JSMN_PRIMITIVE);
				res.spawner.zones[res.spawner.zones_len++] = json_parse_i32(json, item);
			}
			dbg_assert(res.spawner.zones_len == value->size);
			i += value->size;
		}
	}

	return res;
}

struct pinbtjson_res
pinbtjson_handle_spawn_zone(
	str8 json,
	jsmntok_t *tokens,
	i32 index,
	struct alloc alloc)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);

	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		if(json_eq(json, key, str8_lit("mode")) == 0) {
			res.spawn_zone.mode = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("capacity")) == 0) {
			res.spawn_zone.capacity = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("cir")) == 0) {
			struct pinbtjson_res item_res = pinbtjson_handle_col_cir(json, tokens, i);
			res.spawn_zone.type           = PINB_SPAWN_ZONE_TYPE_CIR;
			res.spawn_zone.cir            = item_res.cir;
			i += item_res.token_count - 1;
		} else if(json_eq(json, key, str8_lit("aabb")) == 0) {
			struct pinbtjson_res item_res = pinbtjson_handle_col_aabb(json, tokens, i);
			res.spawn_zone.type           = PINB_SPAWN_ZONE_TYPE_AABB;
			res.spawn_zone.aabb           = item_res.aabb;
			i += item_res.token_count - 1;
		} else if(json_eq(json, key, str8_lit("point")) == 0) {
			res.spawn_zone.type = PINB_SPAWN_ZONE_TYPE_POINT;
		}
	}

	return res;
}

struct pinbtjson_res
pinbtjson_handle_entity(str8 json, jsmntok_t *tokens, i32 index, struct alloc alloc, struct alloc scratch)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);

	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		str8 key_str     = json_str8(json, key);
		str8 value_str   = json_str8(json, value);
		if(json.str[key->start] == '_') {
		} else if(json_eq(json, key, str8_lit("id")) == 0) {
			res.entity.id = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("flags")) == 0) {
			res.entity.flags = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("x")) == 0) {
			res.entity.x = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("y")) == 0) {
			res.entity.y = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("spr")) == 0) {
			dbg_assert(value->type == JSMN_OBJECT);
			struct pinbtjson_res item_res = pinbtjson_handle_spr(json, tokens, i + 1, alloc);
			res.entity.spr                = item_res.spr;
			i += item_res.token_count - 1;
		} else if(json_eq(json, key, str8_lit("rigid_body")) == 0) {
			struct pinbtjson_res item_res = pinbtjson_handle_rigid_body(json, tokens, i + 1, alloc, scratch);
			res.entity.body               = item_res.body;
			i += item_res.token_count - 1;
		} else if(json_eq(json, key, str8_lit("reactive_impulse")) == 0) {
			dbg_assert(value->type == JSMN_OBJECT);
			struct pinbtjson_res item_res = pinbtjson_handle_reactive_impulse(json, tokens, i + 1);
			res.entity.reactive_impulse   = item_res.reactive_impulse;
			i += item_res.token_count - 1;
		} else if(json_eq(json, key, str8_lit("force_field")) == 0) {
			dbg_assert(value->type == JSMN_OBJECT);
			struct pinbtjson_res item_res = pinbtjson_handle_force_field(json, tokens, i + 1);
			res.entity.force_field        = item_res.force_field;
			i += item_res.token_count - 1;
		} else if(json_eq(json, key, str8_lit("attractor")) == 0) {
			dbg_assert(value->type == JSMN_OBJECT);
			struct pinbtjson_res item_res = pinbtjson_handle_attractor(json, tokens, i + 1);
			res.entity.attractor          = item_res.attractor;
			i += item_res.token_count - 1;
		} else if(json_eq(json, key, str8_lit("reactive_sprite_offset")) == 0) {
			dbg_assert(value->type == JSMN_OBJECT);
			struct pinbtjson_res item_res     = pinbtjson_handle_reactive_sprite_offset(json, tokens, i + 1);
			res.entity.reactive_sprite_offset = item_res.reactive_sprite_offset;
			i += item_res.token_count - 1;
		} else if(json_eq(json, key, str8_lit("reactive_animation")) == 0) {
			dbg_assert(value->type == JSMN_OBJECT);
			struct pinbtjson_res item_res = pinbtjson_handle_reactive_animation(json, tokens, i + 1);
			res.entity.reactive_animation = item_res.reactive_animation;
			i += item_res.token_count - 1;
		} else if(json_eq(json, key, str8_lit("plunger")) == 0) {
			dbg_assert(value->type == JSMN_OBJECT);
			struct pinbtjson_res item_res = pinbtjson_handle_plunger(json, tokens, i + 1);
			res.entity.plunger            = item_res.plunger;
			i += item_res.token_count - 1;
		} else if(json_eq(json, key, str8_lit("charged_impulse")) == 0) {
			dbg_assert(value->type == JSMN_OBJECT);
			struct pinbtjson_res item_res = pinbtjson_handle_charged_impulse(json, tokens, i + 1);
			res.entity.charged_impulse    = item_res.charged_impulse;
			i += item_res.token_count - 1;
		} else if(json_eq(json, key, str8_lit("spinner")) == 0) {
			dbg_assert(value->type == JSMN_OBJECT);
			struct pinbtjson_res item_res = pinbtjson_handle_spinner(json, tokens, i + 1);
			res.entity.spinner            = item_res.spinner;
			i += item_res.token_count - 1;
		} else if(json_eq(json, key, str8_lit("bucket")) == 0) {
			dbg_assert(value->type == JSMN_OBJECT);
			struct pinbtjson_res item_res = pinbtjson_handle_bucket(json, tokens, i + 1);
			res.entity.bucket             = item_res.bucket;
			i += item_res.token_count - 1;
		} else if(json_eq(json, key, str8_lit("ball_saver")) == 0) {
			dbg_assert(value->type == JSMN_OBJECT);
			struct pinbtjson_res item_res = pinbtjson_handle_ball_saver(json, tokens, i + 1);
			res.entity.ball_saver         = item_res.ball_saver;
			i += item_res.token_count - 1;
		} else if(json_eq(json, key, str8_lit("flipper")) == 0) {
			dbg_assert(value->type == JSMN_OBJECT);
			struct pinbtjson_res item_res = pinbtjson_handle_flipper(json, tokens, i + 1);
			res.entity.flipper            = item_res.flipper;
			i += item_res.token_count - 1;
		} else if(json_eq(json, key, str8_lit("flip")) == 0) {
			dbg_assert(value->type == JSMN_OBJECT);
			struct pinbtjson_res item_res = pinbtjson_handle_flip(json, tokens, i + 1);
			res.entity.flip               = item_res.flip;
			i += item_res.token_count - 1;
		} else if(json_eq(json, key, str8_lit("gravity")) == 0) {
			dbg_assert(value->type == JSMN_OBJECT);
			struct pinbtjson_res item_res = pinbtjson_handle_gravity(json, tokens, i + 1);
			res.entity.gravity            = item_res.gravity;
			i += item_res.token_count - 1;
		} else if(json_eq(json, key, str8_lit("counter")) == 0) {
			dbg_assert(value->type == JSMN_OBJECT);
			struct pinbtjson_res item_res = pinbtjson_handle_counter(json, tokens, i + 1);
			res.entity.counter            = item_res.counter;
			i += item_res.token_count - 1;
		} else if(json_eq(json, key, str8_lit("collision_layer")) == 0) {
			dbg_assert(value->type == JSMN_OBJECT);
			struct pinbtjson_res item_res = pinbtjson_handle_collision_layer(json, tokens, i + 1);
			res.entity.collision_layer    = item_res.collision_layer;
			i += item_res.token_count - 1;
		} else if(json_eq(json, key, str8_lit("crank_animation")) == 0) {
			dbg_assert(value->type == JSMN_OBJECT);
			struct pinbtjson_res item_res = pinbtjson_handle_crank_animation(json, tokens, i + 1);
			res.entity.crank_animation    = item_res.crank_animation;
			i += item_res.token_count - 1;
		} else if(json_eq(json, key, str8_lit("reset")) == 0) {
			dbg_assert(value->type == JSMN_OBJECT);
			struct pinbtjson_res item_res = pinbtjson_handle_reset(json, tokens, i + 1);
			res.entity.reset              = item_res.reset;
			i += item_res.token_count - 1;
		} else if(json_eq(json, key, str8_lit("animator")) == 0) {
			dbg_assert(value->type == JSMN_OBJECT);
			struct pinbtjson_res item_res = pinbtjson_handle_animator(json, tokens, i + 1, alloc);
			res.entity.animator           = item_res.animator;
			i += item_res.token_count - 1;
		} else if(json_eq(json, key, str8_lit("score_fx_offset")) == 0) {
			dbg_assert(value->type == JSMN_ARRAY);
			res.entity.score_fx_offset.x = json_parse_i32(json, tokens + i + 2);
			res.entity.score_fx_offset.y = json_parse_i32(json, tokens + i + 3);
			i += value->size;
		} else if(json_eq(json, key, str8_lit("sfx_sequences")) == 0) {
			dbg_assert(value->type == JSMN_ARRAY);
			res.entity.sfx_sequences.len   = value->size;
			res.entity.sfx_sequences.items = arr_new(res.entity.sfx_sequences.items, value->size, alloc);
			for(usize j = 0; j < (usize)value->size; ++j) {
				i32 item_index  = i + 2;
				jsmntok_t *item = tokens + item_index;
				dbg_assert(item->type == JSMN_OBJECT);
				struct pinbtjson_res item_res     = pinbtjson_handle_sfx_sequence(json, tokens, item_index, alloc);
				res.entity.sfx_sequences.items[j] = item_res.sfx_sequence;
				i += item_res.token_count;
			}
		} else if(json_eq(json, key, str8_lit("messages")) == 0) {
			dbg_assert(value->type == JSMN_ARRAY);
			res.entity.messages.len   = value->size;
			res.entity.messages.items = arr_new(res.entity.messages.items, value->size, alloc);
			for(usize j = 0; j < (usize)value->size; ++j) {
				i32 item_index  = i + 2;
				jsmntok_t *item = tokens + item_index;
				dbg_assert(item->type == JSMN_OBJECT);
				struct pinbtjson_res item_res = pinbtjson_handle_message(json, tokens, item_index, alloc);
				res.entity.messages.items[j]  = item_res.message;
				i += item_res.token_count;
			}
		} else if(json_eq(json, key, str8_lit("actions")) == 0) {
			dbg_assert(value->type == JSMN_ARRAY);
			res.entity.actions.len   = value->size;
			res.entity.actions.items = arr_new(res.entity.actions.items, value->size, alloc);
			for(usize j = 0; j < (usize)value->size; ++j) {
				i32 item_index  = i + 2;
				jsmntok_t *item = tokens + item_index;
				dbg_assert(item->type == JSMN_OBJECT);
				struct pinbtjson_res item_res = pinbtjson_handle_action(json, tokens, item_index, alloc);
				res.entity.actions.items[j]   = item_res.action;
				i += item_res.token_count;
			}
		} else if(json_eq(json, key, str8_lit("sensor")) == 0) {
			dbg_assert(value->type == JSMN_OBJECT);
			struct pinbtjson_res item_res = pinbtjson_handle_sensor(json, tokens, i + 1, alloc, scratch);
			res.entity.sensor             = item_res.sensor;
			i += item_res.token_count - 1;
		} else if(json_eq(json, key, str8_lit("switch_value")) == 0) {
			dbg_assert(value->type == JSMN_OBJECT);
			struct pinbtjson_res item_res = pinbtjson_handle_switch_value(json, tokens, i + 1, alloc);
			res.entity.switch_value       = item_res.switch_value;
			i += item_res.token_count - 1;
		} else if(json_eq(json, key, str8_lit("switch_list")) == 0) {
			dbg_assert(value->type == JSMN_OBJECT);
			struct pinbtjson_res item_res = pinbtjson_handle_switch_list(json, tokens, i + 1, alloc);
			res.entity.switch_list        = item_res.switch_list;
			i += item_res.token_count - 1;
		} else if(json_eq(json, key, str8_lit("spawner")) == 0) {
			dbg_assert(value->type == JSMN_OBJECT);
			struct pinbtjson_res item_res = pinbtjson_handle_spawner(json, tokens, i + 1, alloc);
			res.entity.spawner            = item_res.spawner;
			i += item_res.token_count - 1;
		} else if(json_eq(json, key, str8_lit("spawn_zone")) == 0) {
			dbg_assert(value->type == JSMN_OBJECT);
			struct pinbtjson_res item_res = pinbtjson_handle_spawn_zone(json, tokens, i + 1, alloc);
			res.entity.spawn_zone         = item_res.spawn_zone;
			i += item_res.token_count - 1;
		}
	}

	return res;
}

struct pinbtjson_res
pinbtjson_handle_physics_props(str8 json, jsmntok_t *tokens, i32 index)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);
	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		str8 key_str     = json_str8(json, key);
		str8 value_str   = json_str8(json, value);
		if(json_eq(json, key, str8_lit("steps")) == 0) {
			res.physics_props.steps = json_parse_i32(json, value);
		} else if(json_eq(json, key, str8_lit("max_translation")) == 0) {
			res.physics_props.max_translation = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("max_rotation")) == 0) {
			res.physics_props.max_rotation = json_parse_f32(json, value) * PI_FLOAT;
		} else if(json_eq(json, key, str8_lit("penetration_correction")) == 0) {
			res.physics_props.penetration_correction = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("penetration_allowance")) == 0) {
			res.physics_props.penetration_allowance = json_parse_f32(json, value);
		}
	}
	return res;
}

struct pinbtjson_res
pinbtjson_handle_flippers_props(str8 json, jsmntok_t *tokens, i32 index)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);
	for(usize i = index + 1; i < index + res.token_count; i += 2) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		str8 key_str     = json_str8(json, key);
		str8 value_str   = json_str8(json, value);
		if(json_eq(json, key, str8_lit("flip_velocity")) == 0) {
			res.flipper_props.flip_velocity = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("release_velocity")) == 0) {
			res.flipper_props.release_velocity = json_parse_f32(json, value);
		} else if(json_eq(json, key, str8_lit("rotation_max_degrees")) == 0) {
			res.flipper_props.rotation_max_turns = json_parse_f32(json, value) * DEG_TO_TURN;
		} else if(json_eq(json, key, str8_lit("rotation_min_degrees")) == 0) {
			res.flipper_props.rotation_min_turns = json_parse_f32(json, value) * DEG_TO_TURN;
		}
	}
	return res;
}

struct pinbtjson_res
pinbtjson_handle_table_props(str8 json, jsmntok_t *tokens, i32 index, struct alloc alloc)
{
	struct pinbtjson_res res = {0};
	jsmntok_t *root          = &tokens[index];
	dbg_assert(root->type == JSMN_OBJECT);
	res.token_count = json_obj_count(json, root);
	for(usize i = index + 1; i < index + res.token_count; i++) {
		jsmntok_t *key   = tokens + i;
		jsmntok_t *value = tokens + i + 1;
		str8 key_str     = json_str8(json, key);
		str8 value_str   = json_str8(json, value);
		if(json_eq(json, key, str8_lit("physics_props")) == 0) {
			struct pinbtjson_res item_res = pinbtjson_handle_physics_props(json, tokens, i + 1);
			res.table_props.physics_props = item_res.physics_props;
			i += item_res.token_count;
		} else if(json_eq(json, key, str8_lit("bg_tex_path")) == 0) {
			dbg_assert(value->type == JSMN_STRING);
			str8 path                   = json_str8(json, value);
			res.table_props.bg_tex_path = make_file_name_with_ext(alloc, path, str8_lit(TEX_EXT));
			i++;
		} else if(json_eq(json, key, str8_lit("flippers_props")) == 0) {
			struct pinbtjson_res item_res  = pinbtjson_handle_flippers_props(json, tokens, i + 1);
			res.table_props.flippers_props = item_res.flipper_props;
			i += item_res.token_count;
		}
	}
	return res;
}

struct pinb_table
pinbtjson_handle_pinbtjson(str8 json, struct alloc alloc, struct alloc scratch)
{
	struct pinb_table res = {0};

	jsmn_parser parser;
	jsmn_init(&parser);
	i32 token_count = jsmn_parse(&parser, (char *)json.str, json.size, NULL, 0);
	jsmn_init(&parser);
	jsmntok_t *tokens = arr_new(tokens, token_count, scratch);
	i32 json_res      = jsmn_parse(&parser, (char *)json.str, json.size, tokens, token_count);
	dbg_assert(json_res == token_count);

	for(usize i = 1; i < (usize)token_count; i++) {
		jsmntok_t *key   = &tokens[i];
		jsmntok_t *value = &tokens[i + 1];
		str8 key_str     = json_str8(json, key);
		str8 value_str   = json_str8(json, value);
		if(json_eq(json, key, str8_lit("version")) == 0) {
			dbg_assert(value->type == JSMN_PRIMITIVE);
			res.version = json_parse_i32(json, value);
			dbg_assert(res.version == 1);
			++i;
		} else if(json_eq(json, key, str8_lit("props")) == 0) {
			dbg_assert(value->type == JSMN_OBJECT);
			struct pinbtjson_res item_res = pinbtjson_handle_table_props(json, tokens, i + 1, alloc);
			res.props                     = item_res.table_props;
			i += item_res.token_count;
		} else if(json_eq(json, key, str8_lit("entities_max_id")) == 0) {
			dbg_assert(value->type == JSMN_PRIMITIVE);
			res.entities_max_id = json_parse_i32(json, value);
			++i;
		} else if(json_eq(json, key, str8_lit("entities_count")) == 0) {
			dbg_assert(value->type == JSMN_PRIMITIVE);
			res.entities_count = json_parse_i32(json, value);
			res.entities       = arr_new(res.entities, res.entities_count, alloc);
			++i;
		} else if(json_eq(json, key, str8_lit("entities")) == 0) {
			dbg_assert(value->type == JSMN_ARRAY);
			for(i32 j = 0; j < value->size; j++) {
				i32 item_index  = i + 2;
				jsmntok_t *item = tokens + item_index;
				dbg_assert(item->type == JSMN_OBJECT);
				struct pinbtjson_res item_res = pinbtjson_handle_entity(json, tokens, item_index, alloc, scratch);
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
	usize mem_size = MMEGABYTE(4);
	u8 *mem_buffer = sys_alloc(NULL, mem_size);
	dbg_assert(mem_buffer != NULL);
	struct marena marena = {0};
	marena_init(&marena, mem_buffer, mem_size);
	struct alloc alloc = marena_allocator(&marena);

	usize scratch_mem_size = MMEGABYTE(10);
	u8 *scratch_mem_buffer = sys_alloc(NULL, scratch_mem_size);
	dbg_assert(scratch_mem_buffer != NULL);
	struct marena scratch_marena = {0};
	marena_init(&scratch_marena, scratch_mem_buffer, scratch_mem_size);
	struct alloc scratch = marena_allocator(&scratch_marena);

	str8 json = {0};
	json_load(in_path, alloc, &json);
	struct pinb_table table = pinbtjson_handle_pinbtjson(json, alloc, scratch);

	str8 out_file_path = make_file_name_with_ext(alloc, out_path, str8_lit(PINB_EXT));

	void *out_file;
	if(!(out_file = sys_file_open_w(out_file_path))) {
		log_error("pinb-gen", "can't open file %s for writing!", out_file_path.str);
		return -1;
	}

	struct ser_writer w = {.f = out_file};
	pinb_write(&w, table);
	sys_file_close(out_file);

#if DEBUG
	{
		struct sys_full_file_res io_res = sys_load_full_file(out_file_path, scratch);
		struct ser_reader r             = {
						.data = io_res.data,
						.len  = io_res.size,
        };
		struct ser_value obj = ser_read(&r);
	}
#endif

	sys_free(mem_buffer);
	sys_free(scratch_mem_buffer);
	log_info("pinb-gen", "%s -> %s\n", in_path.str, out_file_path.str);

	return 1;
}
