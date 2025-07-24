#pragma once

#include "pinb/pinb.h"
#include "serialize/serialize.h"

i32 pinb_write(struct ser_writer *w, struct pinb_table pinb);
i32 pinb_read(struct ser_reader *r, struct pinb_table *table, struct alloc alloc);
i32 pinb_inspect(str8 path, struct ser_reader *r, struct alloc alloc);

void pinb_entity_spr_write(struct ser_writer *w, struct pinb_entity entity);
void pinb_counter_write(struct ser_writer *w, struct pinb_counter value);
void pinb_sensor_write(struct ser_writer *w, struct pinb_sensor value);
void pinb_reactive_impulse_write(struct ser_writer *w, struct pinb_reactive_impulse value);
void pinb_force_field_write(struct ser_writer *w, struct pinb_force_field value);
void pinb_attractor_write(struct ser_writer *w, struct pinb_attractor value);
void pinb_reactive_sprite_offset_write(struct ser_writer *w, struct pinb_reactive_sprite_offset value);
void pinb_reactive_animation_write(struct ser_writer *w, struct pinb_reactive_animation value);
void pinb_charged_impulse_write(struct ser_writer *w, struct pinb_charged_impulse value);
void pinb_plunger_write(struct ser_writer *w, struct pinb_plunger value);
void pinb_spinner_write(struct ser_writer *w, struct pinb_spinner value);
void pinb_bucket_write(struct ser_writer *w, struct pinb_bucket value);
void pinb_ball_saver_write(struct ser_writer *w, struct pinb_ball_saver value);
void pinb_flipper_write(struct ser_writer *w, struct pinb_flipper value);
void pinb_flip_write(struct ser_writer *w, struct pinb_flip value);
void pinb_gravity_write(struct ser_writer *w, struct pinb_gravity value);
void pinb_collision_layer_write(struct ser_writer *w, struct pinb_collision_layer value);
void pinb_crank_animation_write(struct ser_writer *w, struct pinb_crank_animation value);
void pinb_reset_write(struct ser_writer *w, struct pinb_reset value);
void pinb_animator_write(struct ser_writer *w, struct pinb_animator value);
void pinb_animator_transitions_write(struct ser_writer *w, struct pinb_animator_transitions value);
void pinb_switch_value_write(struct ser_writer *w, struct pinb_switch value);
void pinb_switch_list_write(struct ser_writer *w, struct pinb_switch_list value);
void pinb_spawner_write(struct ser_writer *w, struct pinb_spawner value);
void pinb_sfx_sequences_write(struct ser_writer *w, struct pinb_sfx_sequences value);
void pinb_sfx_sequence_write(struct ser_writer *w, struct pinb_sfx_sequence value);
void pinb_messages_write(struct ser_writer *w, struct pinb_messages value);
void pinb_message_write(struct ser_writer *w, struct pinb_message value);
void pinb_actions_write(struct ser_writer *w, struct pinb_actions value);
void pinb_action_write(struct ser_writer *w, struct pinb_action value);

struct pinb_entity pinb_entity_read(struct ser_reader *r, struct ser_value obj, struct alloc alloc);
struct pinb_table_props pinb_table_props_read(struct ser_reader *r, struct ser_value obj);
struct pinb_sensor pinb_sensor_read(struct ser_reader *r, struct ser_value obj);
struct pinb_switch pinb_switch_value_read(struct ser_reader *r, struct ser_value obj);
struct pinb_switch_list pinb_switch_list_read(struct ser_reader *r, struct ser_value obj);
struct pinb_sfx_sequence pinb_sfx_sequence_read(struct ser_reader *r, struct ser_value obj, struct alloc alloc);
struct pinb_message pinb_message_read(struct ser_reader *r, struct ser_value obj, struct alloc alloc);
struct pinb_action pinb_action_read(struct ser_reader *r, struct ser_value obj);
struct pinb_animator pinb_animator_read(struct ser_reader *r, struct ser_value obj, struct alloc alloc);
struct pinb_spawner pinb_spawner_read(struct ser_reader *r, struct ser_value obj);
