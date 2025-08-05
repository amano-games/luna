/// <reference types="@mapeditor/tiled-api" />

import {
  flipPolygon,
  getCol,
  getColType,
  getObjectColType,
  getTileObjects,
} from "./cols";
import {
  Animator,
  COL_TYPE_NONE,
  COL_TYPE_AABB,
  COL_TYPE_CAPSULE,
  COL_TYPE_CIR,
  COL_TYPE_POINT,
  COL_TYPE_POLY,
  Entity,
  Flip,
  Flipper,
  FlipperProps,
  Gravity,
  PhysicsProps,
  PinballTable,
  Plunger,
  ChargedImpulse,
  ReactiveImpulse,
  ReactiveSpriteOffset,
  RigidBody,
  Sensor,
  SfxSequence,
  Message,
  Sprite,
  SwitchValue,
  EntityList,
  TableProperties,
  GameAction,
  ReactiveAnimation,
  SfxToAnimation,
  Spinner,
  ScoreFXOffset,
  AnimatorTransition,
  CollisionShape,
  Reset,
  Counter,
  ForceField,
  Attractor,
  Bucket,
  CrankAnimation,
  SpriteLayer,
  CollisionLayer,
  LayerProps,
  BallSaver,
  Spawner,
  SpawnZoneRef,
  SpawnZone,
} from "./types";
import { getImgPath } from "./utils";

export function getPropByType(object: MapObject, typeName: string) {
  const props = object.properties();
  const entry = Object.entries(props).find(([_, value]) => {
    const prop = value as PropertyValue;
    if (prop.typeName === typeName) {
      return true;
    }
  });
  if (entry == null) {
    return undefined;
  }
  const [_key, res] = entry;
  return res as PropertyValue;
}

function getPos(object: MapObject) {
  const collisioType = getObjectColType(object);
  const isTile = object.tile != undefined;
  // Tiles have their pivot at their bottom left
  switch (collisioType) {
    case COL_TYPE_CIR:
      if (isTile) {
        const [first] = getTileObjects(object);
        const ax = object.x;
        const ay = object.y - object.height;
        const bx = first.x + first.width / 2;
        const by = first.y + first.height / 2;
        return [ax + bx, ay + by];
      } else {
        return [object.x + object.width / 2, object.y + object.height / 2];
      }
    case COL_TYPE_POLY:
      return [object.x, object.y + (isTile ? -object.height : 0)];
    case COL_TYPE_AABB:
      return [object.x, object.y + (isTile ? -object.height : 0)];
    case COL_TYPE_POINT: {
      if (isTile) {
        const [first] = getTileObjects(object);
        return [object.x + first.x, object.y + first.y];
      } else {
        return [object.x, object.y];
      }
    }
    case COL_TYPE_CAPSULE: {
      if (!isTile) {
        tiled.error(
          `Object with id ${object.id} is not a capsule but was identified as one`,
          null,
        );
      }
      const [first] = getTileObjects(object);
      const r = first.width / 2;
      let x = object.x + first.x + r;
      let y = object.y - object.height + first.y + r;
      if (object.tileFlippedHorizontally) {
        x = object.x + object.width - (first.x + r);
      }
      if (object.tileFlippedVertically) {
        tiled.warn(
          `object with id: ${object.id} flipped vertically not supported on capsule type collisions`,
          null,
        );
      }
      return [x, y];
    }
    case COL_TYPE_NONE:
      return [object.x, object.y + (isTile ? -object.height : 0)];
  }
}

function getRigidBody(object: MapObject, prop: PropertyValue) {
  const value = prop.value as object;
  const collisionShape = getCol(object) as CollisionShape;

  const res: RigidBody = {
    angular_damping: value["angular_damping"],
    dynamic_friction: value["dynamic_friction"],
    flags: value["flags"].value,
    linear_damping: value["linear_damping"],
    mass: value["mass"],
    restitution: value["restitution"],
    static_friction: value["static_friction"],
    collision_shape: collisionShape,
  };
  return res;
}

function getSensor(object: MapObject, prop: PropertyValue) {
  const value = prop.value as object;

  const res: Sensor = {
    is_enabled: value["is_enabled"],
    collision_shape: getCol(object),
  };
  return res;
}

function getSprite(
  object: MapObject,
  x: number,
  y: number,
  layerProps: LayerProps,
) {
  if (!object.tile) {
    return undefined;
  }
  const [first] = getTileObjects(object);
  const colType = getColType(first?.shape);
  const offset = [0, 0];
  if (colType == COL_TYPE_CIR) {
    offset[0] = object.x - x;
    offset[1] = object.y - object.height - y;
  }
  const { imageFileName } = object.tile;
  const path = getImgPath(imageFileName);
  let flip = 0;
  flip = flip | (object.tileFlippedHorizontally ? 1 : 0);
  flip = flip | (object.tileFlippedVertically ? 2 : 0);
  const layer = layerProps.sprite_layer?.layer
    ? layerProps.sprite_layer.layer
    : 0;
  const y_sort = layerProps.sprite_layer?.y_sort
    ? layerProps.sprite_layer.y_sort
    : false;
  const res = {
    path,
    flip,
    offset,
    layer,
    y_sort,
  } as Sprite;
  return res;
}

function getReactiveImpulse(_object: MapObject, prop: PropertyValue) {
  const value = prop.value as object;

  const res: ReactiveImpulse = {
    magnitude: value["magnitude"],
    normalize: value["normalize"],
  };
  return res;
}

function getReactiveSpriteOffset(_object: MapObject, prop: PropertyValue) {
  const value = prop.value as object;
  const ref = Number(value["ref"]?.id) || 0;

  const res: ReactiveSpriteOffset = {
    delay: value["delay"],
    magnitude: value["magnitude"],
    ref: ref,
  };
  return res;
}

function getReactiveAnimation(_object: MapObject, prop: PropertyValue) {
  const value = prop.value as object;

  const res: ReactiveAnimation = {
    animation_index: value["animation_index"],
  };
  return res;
}

function getPlunger(_object: MapObject, prop: PropertyValue) {
  const value = prop.value as object;

  const res: Plunger = {
    charge_force_max: value["charge_force_max"],
    charge_force_min: value["charge_force_min"],
    release_force_max: value["release_force_max"],
    release_force_min: value["release_force_min"],
  };
  return res;
}

function getChargedImpulse(_object: MapObject, prop: PropertyValue) {
  const value = prop.value as object;

  const res: ChargedImpulse = {
    angle_degrees: value["angle_degrees"],
    magnitude: value["magnitude"],
    charge_speed: value["speed_charge"],
    release_speed: value["speed_release"],
    reset_target: value["reset_target"],
    auto_shoot: value["auto_shoot"],
    auto_shoot_hold: value["auto_shoot_hold"],
  };
  return res;
}

function getSpinner(_object: MapObject, prop: PropertyValue) {
  const value = prop.value as object;

  const res: Spinner = {
    damping: value["damping"],
    spin_force: value["spin_force"],
    stop_threshold: value["stop_threshold"],
  };
  return res;
}

function getBucket(_object: MapObject, prop: PropertyValue) {
  const value = prop.value as object;

  const res: Bucket = {
    animation_shoot: value["animation_shoot"],
    animation_on: value["animation_on"],
    animation_off: value["animation_off"],
    impulse_angle_degrees: value["impulse_angle_degrees"],
    impulse_magnitude: value["impulse_magnitude"],
    delay: value["delay"],
  };
  return res;
}

function getFlip(_object: MapObject, prop: PropertyValue) {
  const value = prop.value as object;
  const res: Flip = {
    type: value["type"].value,
    is_enabled: value["is_enabled"],
  };
  return res;
}

function getFlipper(_object: MapObject, prop: PropertyValue) {
  const value = prop.value as object;
  const res: Flipper = {
    velocity_easing_function: value["velocity_easing_function"].value,
    velocity_scale: value["velocity_scale"],
    velocity_radius_min: value["velocity_radius_min"],
    velocity_radius_max: value["velocity_radius_max"],
  };
  return res;
}

function getSpriteOffset(_object: MapObject, prop: PropertyValue) {
  const value = prop.value as object;

  const res = [value["x"], value["y"]] as [number, number];
  return res;
}

function getGravity(_object: MapObject, prop: PropertyValue) {
  const value = prop.value as object;
  const res: Gravity = {
    value: value["value"],
  };
  return res;
}

function getCounter(_object: MapObject, prop: PropertyValue) {
  const value = prop.value as object;
  const res: Counter = {
    max: value["max"],
    min: value["min"],
    resolution: value["resolution"],
    value: value["value"],
  };
  return res;
}

function getBallSaver(_object: MapObject, prop: PropertyValue) {
  const value = prop.value as object;
  const res: BallSaver = {
    duration: value["duration"],
    save_delay: value["save_delay"],
    is_enabled: value["is_enabled"],
  };
  return res;
}

function getCrankAnimation(_object: MapObject, prop: PropertyValue) {
  const value = prop.value as object;
  const res: CrankAnimation = {
    interval: value["interval"],
  };
  return res;
}

function getSfxSequence(_object: MapObject, prop: PropertyValue) {
  const value = prop.value as object;

  const res: SfxSequence = {
    type: value["type"].value,
    vol_min: value["vol_min"],
    vol_max: value["vol_max"],
    pitch_min: value["pitch_min"],
    pitch_max: value["pitch_max"],
    reset_time: value["reset_time"],
    clips: value["clips"].split("\n"),
  };
  return res;
}

function getMessage(_object: MapObject, prop: PropertyValue) {
  const value = prop.value as object;

  const res: Message = {
    sequence_type: value["sequence_type"].value,
    sequence_reset_time: value["sequence_reset_time"],
    hide_time: value["hide_time"],
    text: value["text"].split("\n"),
  };
  return res;
}

function getSfxToAnimation(_object: MapObject, prop: PropertyValue) {
  const value = prop.value as object;

  const res: SfxToAnimation = {
    animation: value["animation"],
    sfx: value["sfx"],
  };
  return res;
}

function getAnimator(_object: MapObject, prop: PropertyValue) {
  const value = prop.value as object;

  const res: Animator = {
    play_on_start: value["play_on_start"],
    initial_animation: value["initial_animation"],
    transitions: [],
  };
  return res;
}

function getAnimatorTransition(_object: MapObject, prop: PropertyValue) {
  const value = prop.value as object;

  const res: AnimatorTransition = [value["from"], value["to"]];
  return res;
}

function getScoreFxOffset(_object: MapObject, prop: PropertyValue) {
  const value = prop.value as object;

  const res: ScoreFXOffset = [value["x"], value["y"]];

  return res;
}

function getSwitchValue(_object: MapObject, prop: PropertyValue) {
  const value = prop.value as object;

  const res: SwitchValue = {
    is_enabled: value["is_enabled"],
    value: value["value"],
    animation_on: value["animation_on"],
    animation_off: value["animation_off"],
  };
  return res;
}

function getEntityList(_object: MapObject, prop: PropertyValue) {
  const value = prop.value as object;
  const res: EntityList = {
    prev: Number(value["prev"].id),
    next: Number(value["next"].id),
  };
  return res;
}

function getReset(_object: MapObject, prop: PropertyValue) {
  const value = prop.value as object;

  const res: Reset = {
    flags: value["flags"].value,
  };
  return res;
}

function getForceField(_object: MapObject, prop: PropertyValue) {
  const value = prop.value as object;

  const res: ForceField = {
    angle_degrees: value["angle_degrees"],
    magnitude: value["magnitude"],
    is_enabled: value["is_enabled"],
  };
  return res;
}

function getAttractor(_object: MapObject, prop: PropertyValue) {
  const value = prop.value as object;

  const res: Attractor = {
    flags: value["flags"].value,
    offset: [value["offset_x"], value["offset_y"]],
    radius: value["radius"],
    force: value["force"],
    damping: value["damping"],
    distance_threshold: value["distance_threshold"],
  };
  return res;
}

function getAction(object: MapObject, key: string, prop: PropertyValue) {
  const value = prop.value as object;
  const action_ref = Number(value["action_ref"].id) || object.id;
  const res: GameAction = {
    _name: key,
    debug: value["debug"],
    action_argument: value["action_argument"],
    action_ref: action_ref,
    action_type: value["action_type"].value,
    action_delay: value["action_delay"],
    action_cooldown: value["action_cooldown"],
    event_condition: value["event_condition"],
    event_condition_type: value["event_condition_type"]?.value,
    event_type: value["event_type"].value,
  };
  return res;
}

function getSpriteLayer(prop: PropertyValue) {
  const value = prop.value as object;
  const res: SpriteLayer = {
    layer: value["layer"].value,
    y_sort: value["y_sort"],
    y_sort_offset: value["y_sort_offset"],
  };
  return res;
}

function getCollisionLayer(prop: PropertyValue) {
  const value = prop.value as object;
  const res: CollisionLayer = {
    layer: value["layer"].value,
  };
  return res;
}

function getSpawner(prop: PropertyValue) {
  const value = prop.value as object;
  const ref = Number(value["ref"]?.id) || 0;
  const res: Spawner = {
    ref: ref,
    type: value["type"].value,
    zones: [],
  };
  return res;
}

function getSpawnZoneRef(prop: PropertyValue) {
  const value = prop.value as object;
  const ref = Number(value["ref"]?.id) || 0;
  const res: SpawnZoneRef = {
    ref: ref,
  };
  return res;
}

function getSpawnZone(object: MapObject, prop: PropertyValue) {
  const value = prop.value as object;
  const collisionShape = getCol(object) as CollisionShape;

  const res: SpawnZone = {
    mode: value["mode"].value,
    capacity: value["capacity"],
    point: collisionShape.point,
    cir: collisionShape.cir,
    aabb: collisionShape.aabb,
  };
  return res;
}

function handleObjectLayer(layer: Layer, layer_index: number) {
  const res = [];
  if (!layer.isObjectLayer) {
    return res;
  }

  const layerProps: LayerProps = Object.entries(layer.properties()).reduce(
    (acc, [_key, value]) => {
      const prop = value as PropertyValue;
      switch (prop.typeName) {
        case "sprite_layer": {
          return {
            ...acc,
            sprite_layer: getSpriteLayer(prop),
          };
        }
        case "collision_layer": {
          return {
            ...acc,
            collision_layer: getCollisionLayer(prop),
          };
        }
      }
      return acc;
    },
    {} as LayerProps,
  );

  const objectGroup = layer as ObjectGroup;

  return objectGroup.objects
    .map((item) => {
      const [x, y] = getPos(item);
      const res: Entity = Object.entries(item.properties()).reduce(
        (acc, [key, value]) => {
          const prop = value as PropertyValue;
          switch (prop.typeName) {
            case "entity_flags": {
              return {
                ...acc,
                flags: prop.value,
              };
            }
            case "rigid_body":
              return {
                ...acc,
                rigid_body: getRigidBody(item, prop),
              };
            case "plunger":
              return {
                ...acc,
                plunger: getPlunger(item, prop),
              };
            case "charged_impulse":
              return {
                ...acc,
                charged_impulse: getChargedImpulse(item, prop),
              };
            case "spinner":
              return {
                ...acc,
                spinner: getSpinner(item, prop),
              };
            case "bucket":
              return {
                ...acc,
                bucket: getBucket(item, prop),
              };
            case "flipper":
              return {
                ...acc,
                flipper: getFlipper(item, prop),
              };
            case "flip":
              return {
                ...acc,
                flip: getFlip(item, prop),
              };
            case "reactive_impulse":
              return {
                ...acc,
                reactive_impulse: getReactiveImpulse(item, prop),
              };
            case "reactive_sprite_offset":
              return {
                ...acc,
                reactive_sprite_offset: getReactiveSpriteOffset(item, prop),
              };
            case "reactive_animation":
              return {
                ...acc,
                reactive_animation: getReactiveAnimation(item, prop),
              };
            case "sprite_offset":
              if (acc.spr != null) {
                const spr = {
                  ...acc.spr,
                  offset: getSpriteOffset(item, prop),
                };
                return {
                  ...acc,
                  spr,
                };
              }
              return acc;
            case "sprite_layer":
              const sprite_layer = getSpriteLayer(prop);
              if (acc.spr != null) {
                const spr = {
                  ...acc.spr,
                  ...sprite_layer,
                };
                return {
                  ...acc,
                  spr,
                };
              }
              return acc;
            case "collision_layer":
              return {
                ...acc,
                collision_layer: getCollisionLayer(prop),
              };
            case "gravity":
              return {
                ...acc,
                gravity: getGravity(item, prop),
              };
            case "counter":
              return {
                ...acc,
                counter: getCounter(item, prop),
              };
            case "ball_saver":
              return {
                ...acc,
                ball_saver: getBallSaver(item, prop),
              };
            case "crank_animation":
              return {
                ...acc,
                crank_animation: getCrankAnimation(item, prop),
              };
            case "sensor":
              return {
                ...acc,
                sensor: getSensor(item, prop),
              };
            case "switch_value":
              return {
                ...acc,
                switch_value: getSwitchValue(item, prop),
              };
            case "entity_list":
              return {
                ...acc,
                entity_list: getEntityList(item, prop),
              };
            case "reset":
              return {
                ...acc,
                reset: getReset(item, prop),
              };
            case "force_field":
              return {
                ...acc,
                force_field: getForceField(item, prop),
              };
            case "attractor":
              return {
                ...acc,
                attractor: getAttractor(item, prop),
              };
            case "sfx_sequence":
              if (acc.sfx_sequences == null) {
                acc.sfx_sequences = [];
              }
              return {
                ...acc,
                sfx_sequences: [
                  ...acc.sfx_sequences,
                  getSfxSequence(item, prop),
                ],
              };
            case "message":
              if (acc.messages == null) {
                acc.messages = [];
              }
              return {
                ...acc,
                messages: [...acc.messages, getMessage(item, prop)],
              };
            case "sfx_to_animation":
              if (acc.sfx_to_animation == null) {
                acc.sfx_to_animation = [];
              }
              return {
                ...acc,
                sfx_to_animation: [
                  ...acc.sfx_to_animation,
                  getSfxToAnimation(item, prop),
                ],
              };
            case "action":
              if (acc.actions == null) {
                acc.actions = [];
              }
              return {
                ...acc,
                actions: [...acc.actions, getAction(item, key, prop)],
              };
            case "animation":
              if (acc.animator == null) {
                return {
                  ...acc,
                  animator: {
                    play_on_start: false,
                    initial_animation: 1,
                    transitions: [],
                  },
                };
              } else {
                return acc;
              }
            case "animator_transition":
              if (acc.animator == null) {
                acc.animator = {
                  play_on_start: false,
                  initial_animation: 1,
                  transitions: [],
                };
              }
              return {
                ...acc,
                animator: {
                  ...acc.animator,
                  transitions: [
                    ...acc.animator.transitions,
                    getAnimatorTransition(item, prop),
                  ],
                },
              };
            case "animator":
              if (acc.animator == null) {
                return {
                  ...acc,
                  animator: getAnimator(item, prop),
                };
              } else {
                return {
                  ...acc,
                  animator: {
                    ...getAnimator(item, prop),
                    transitions: [...acc.animator.transitions],
                  },
                };
              }
            case "score_fx_offset":
              return {
                ...acc,
                score_fx_offset: getScoreFxOffset(item, prop),
              };
            case "spawn_zone_ref":
              const zoneRef = getSpawnZoneRef(prop);
              if (acc.spawner == null) {
                const spawner: Spawner = {
                  ref: 0,
                  type: 0,
                  zones: [zoneRef.ref],
                };
                return {
                  ...acc,
                  spawner: spawner,
                };
              } else {
                return {
                  ...acc,
                  spawner: {
                    ...acc.spawner,
                    zones: [...acc.spawner.zones, zoneRef.ref],
                  },
                };
              }
            case "spawner":
              const spawner = getSpawner(prop);
              if (acc.spawner == null) {
                return {
                  ...acc,
                  spawner: spawner,
                };
              } else {
                return {
                  ...acc,
                  spawner: {
                    ...spawner,
                    zones: [...acc.spawner.zones, ...spawner.zones],
                  },
                };
              }
            case "spawn_zone":
              return {
                ...acc,
                spawn_zone: getSpawnZone(item, prop),
              };
            default: {
              return acc;
            }
          }
        },
        {
          _name: item.name,
          _layer: layer_index,
          _layer_name: layer.name,
          id: item.id,
          x,
          y,
          spr: getSprite(item, x, y, layerProps),
          collision_layer: layerProps.collision_layer,
        } as Entity,
      );
      if (res == null) {
        return null;
      }

      // TODO: move to get collision
      if (res.spr != null) {
        if (res.rigid_body?.collision_shape?.poly != null) {
          if (res.spr.flip != 0) {
            res.rigid_body.collision_shape.poly = flipPolygon(
              res.rigid_body.collision_shape.poly,
              res.spr.flip,
              item.width,
              item.height,
            );
          }
        }
      }
      return res;
    })
    .filter(Boolean);
}

function getBgTexPath(prop: PropertyValue) {
  const value = prop.value as object;
  const res = getImgPath(value["image"].localFile);
  return res;
}

function getFlippersProps(prop: PropertyValue) {
  const value = prop.value as object;
  const res: FlipperProps = {
    flip_velocity: value["flip_velocity"],
    release_velocity: value["release_velocity"],
    rotation_max_degrees: value["rotation_max_degrees"],
    rotation_min_degrees: value["rotation_min_degrees"],
  };
  return res;
}

function getPhysicsProps(prop: PropertyValue) {
  const value = prop.value as object;
  const res: PhysicsProps = {
    steps: Math.floor(value["steps"]),
    max_rotation: value["max_rotation"],
    max_translation: value["max_translation"],
    penetration_allowance: value["penetration_allowance"],
    penetration_correction: value["penetration_correction"],
  };
  return res;
}

function getTableProps(map: TileMap) {
  const res: TableProperties = Object.entries(map.properties()).reduce(
    (acc, [_key, value]) => {
      const prop = value as PropertyValue;
      switch (prop.typeName) {
        case "bg_image":
          return {
            ...acc,
            bg_tex_path: getBgTexPath(prop),
          };
        case "flippers_props":
          return {
            ...acc,
            flippers_props: getFlippersProps(prop),
          };
        case "physics_props": {
          return {
            ...acc,
            physics_props: getPhysicsProps(prop),
          };
        }
      }
      return acc;
    },
    {},
  );
  return res;
}

const pinbJSONFormat: ScriptedMapFormat = {
  name: "Pinball JSON table",
  extension: "pinbjson",

  write: function (map: TileMap, fileName: string) {
    const props = getTableProps(map);

    const m: PinballTable = {
      version: 1,
      props,
      entities_count: 0,
      entities_max_id: 0,
      entities: [],
    };

    for (let i = 0; i < map.layerCount; ++i) {
      const layer = map.layerAt(i);
      if (layer.isObjectLayer) {
        const entities = handleObjectLayer(layer as ObjectGroup, i);
        m.entities = [...m.entities, ...entities];
      }
    }
    m.entities_count = m.entities.length;
    m.entities_max_id = m.entities.reduce((acc, curr) => {
      return Math.max(acc, curr.id);
    }, 0);

    var file = new TextFile(fileName, TextFile.WriteOnly);
    file.write(JSON.stringify(m, null, 2));
    file.commit();
    tiled.log(`Exported PINB. entities count: ${m.entities_count}`);
    return undefined;
  },
};

tiled.registerMapFormat("pinbjson", pinbJSONFormat);
