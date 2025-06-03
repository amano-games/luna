export const COL_TYPE_NONE = 0;
export const COL_TYPE_CIR = 1;
export const COL_TYPE_AABB = 2;
export const COL_TYPE_POLY = 3;
export const COL_TYPE_CAPSULE = 4;
export const COL_TYPE_POINT = 5;

export interface Cir {
  r: number;
  x: number;
  y: number;
}

export type Capsule = [Cir, Cir];

export type AABB = [number, number, number, number];

export type Poly = number[];

export interface CollisionShape {
  poly?: Poly;
  aabb?: AABB;
  cir?: Cir;
  capsule?: Capsule;
}

export interface RigidBody {
  angular_damping: number;
  dynamic_friction: number;
  flags: number;
  linear_damping: number;
  mass: number;
  restitution: number;
  static_friction: number;
  collision_shape?: CollisionShape;
}

export interface ChargedImpulse {
  angle_degrees: number;
  magnitude: number;
  charge_speed: number;
  release_speed: number;
  reset_target: boolean;
  auto_shoot: boolean;
}

export interface Bucket {
  animation_shoot: number;
  impulse_angle_degrees: number;
  impulse_magnitude: number;
  delay: number;
}

export interface Plunger {
  charge_force_max: number;
  charge_force_min: number;
  release_force_max: number;
  release_force_min: number;
}

export interface Flip {
  type: number;
  is_enabled: boolean;
}

export interface Flipper {
  velocity_easing_function: number;
  velocity_radius_max: number;
  velocity_radius_min: number;
  velocity_scale: number;
}

export interface ReactiveImpulse {
  magnitude: number;
  normalize: boolean;
}

export interface ReactiveSpriteOffset {
  delay: number;
  magnitude: number;
  ref: number;
}

export interface ReactiveAnimation {
  animation_index: number;
}

export type AnimatorTransition = [number, number];

export interface Animator {
  play_on_start: boolean;
  initial_animation: number;
  transitions: AnimatorTransition[];
}

export interface Sprite {
  path: string;
  offset: [number, number];
  flip: number;
}

export interface Gravity {
  value: number;
}

export interface Counter {
  max: number;
  min: number;
  resolution: number;
  value: number;
}

export interface CrankAnimation {
  interval: number;
}

export interface SfxSequence {
  clips: string[];
  pitch_max: number;
  pitch_min: number;
  reset_time: number;
  type: number;
  vol_max: number;
  vol_min: number;
}

export interface Message {
  text: string[];
  hide_time: number;
  sequence_reset_time: number;
  sequence_type: number;
}

export interface Sensor {
  is_enabled: boolean;
  collision_shape: CollisionShape;
}

export interface SwitchValue {
  is_enabled: boolean;
  value: boolean;
  animation_on: number;
  animation_off: number;
}

export interface SwitchList {
  next: number;
  prev: number;
}

export interface Spinner {
  damping: number;
  spin_force: number;
  stop_threshold: number;
}

export interface ForceField {
  angle_degrees: number;
  magnitude: number;
  is_enabled: boolean;
}

export interface Attractor {
  flags: number;
  offset: [number, number];
  radius: number;
  force: number;
  damping: number;
  distance_threshold: number;
}

export interface GameAction {
  _name?: string;
  debug: boolean;
  action_argument: number;
  action_ref: number;
  action_type: number;
  action_delay: number;
  event_condition: number;
  event_condition_type: number;
  event_type: number;
}

export interface SfxToAnimation {
  animation: number;
  sfx: number;
}

export type ScoreFXOffset = [number, number];

export interface Reset {
  flags: number;
}

export interface Entity {
  _name: string;
  _layer: number;
  _layer_name: string;
  id: number;
  x: number;
  y: number;
  rigid_body?: RigidBody;
  charged_impulse?: ChargedImpulse;
  plunger?: Plunger;
  spr?: Sprite;
  reactive_impulse?: ReactiveImpulse;
  reactive_sprite_offset?: ReactiveSpriteOffset;
  reactive_animation?: ReactiveAnimation;
  flip?: Flip;
  flipper?: Flipper;
  gravity?: Gravity;
  animator?: Animator;
  sensor?: Sensor;
  switch_value?: SwitchValue;
  switch_list?: SwitchList;
  counter?: Counter;
  crank_animation?: CrankAnimation;
  score_fx_offset?: ScoreFXOffset;
  spinner?: Spinner;
  bucket?: Bucket;
  force_field?: ForceField;
  attractor?: Attractor;
  actions?: GameAction[];
  sfx_sequences?: SfxSequence[];
  messages?: Message[];
  sfx_to_animation?: SfxToAnimation[];
  reset?: Reset;
}

export interface PhysicsProps {
  steps: number;
  max_rotation: number;
  max_translation: number;
  penetration_allowance: number;
  penetration_correction: number;
}

export interface FlipperProps {
  flip_velocity: number;
  rotation_max_degrees: number;
  rotation_min_degrees: number;
  release_velocity: number;
}

export interface TableProperties {
  flippers_props?: FlipperProps;
}

export interface PinballTable {
  version: number;
  entities_count: number;
  entities: Entity[];
  entities_max_id: number;
  props: TableProperties;
}
