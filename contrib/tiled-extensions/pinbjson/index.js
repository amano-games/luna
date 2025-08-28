(() => {
  var __defProp = Object.defineProperty;
  var __defProps = Object.defineProperties;
  var __getOwnPropDescs = Object.getOwnPropertyDescriptors;
  var __getOwnPropSymbols = Object.getOwnPropertySymbols;
  var __hasOwnProp = Object.prototype.hasOwnProperty;
  var __propIsEnum = Object.prototype.propertyIsEnumerable;
  var __defNormalProp = (obj, key, value) => key in obj ? __defProp(obj, key, { enumerable: true, configurable: true, writable: true, value }) : obj[key] = value;
  var __spreadValues = (a, b) => {
    for (var prop in b || (b = {}))
      if (__hasOwnProp.call(b, prop))
        __defNormalProp(a, prop, b[prop]);
    if (__getOwnPropSymbols)
      for (var prop of __getOwnPropSymbols(b)) {
        if (__propIsEnum.call(b, prop))
          __defNormalProp(a, prop, b[prop]);
      }
    return a;
  };
  var __spreadProps = (a, b) => __defProps(a, __getOwnPropDescs(b));

  // types.ts
  var COL_TYPE_NONE = 0;
  var COL_TYPE_CIR = 1;
  var COL_TYPE_AABB = 2;
  var COL_TYPE_POLY = 3;
  var COL_TYPE_CAPSULE = 4;
  var COL_TYPE_POINT = 5;

  // cols.ts
  function getObjectColType(object) {
    if (object == null) {
      return void 0;
    }
    if (object.tile) {
      const [first, second] = getTileObjects(object);
      const colType2 = getColType(first == null ? void 0 : first.shape, second == null ? void 0 : second.shape);
      return colType2;
    }
    const colType = getColType(object.shape);
    return colType;
  }
  function getColType(a, b) {
    if (a == null) {
      return COL_TYPE_NONE;
    }
    const aType = Number(a);
    switch (aType) {
      case 0:
        return COL_TYPE_AABB;
      case 1:
        return COL_TYPE_POLY;
      case 3:
        if (Number(b) === 3) {
          return COL_TYPE_CAPSULE;
        }
        return COL_TYPE_CIR;
      case 5:
        return COL_TYPE_POINT;
      default:
        tiled.log(`Shape type unknown ${aType}`);
    }
    return COL_TYPE_NONE;
  }
  function flipPolygon(arr, flip, width, height) {
    const verts = [...arr];
    const flipX = (flip & 1) != 0;
    const flipY = (flip & 2) != 0;
    for (let i = 0; i < arr.length; i += 2) {
      const x = verts[i];
      const y = verts[i + 1];
      verts[i] = flipX ? -x + width : x;
      verts[i + 1] = flipY ? -y + height : y;
    }
    return verts;
  }
  function getPolygon(object, isTile = false) {
    if (getColType(object.shape) !== COL_TYPE_POLY) {
      return void 0;
    }
    const res = object.polygon.reduce((acc, curr) => {
      return [
        ...acc,
        ...[curr.x + (isTile ? object.x : 0), curr.y + (isTile ? object.y : 0)]
      ];
    }, []);
    return res;
  }
  function getAABB(object, isTile = false) {
    if (getColType(object.shape) !== COL_TYPE_AABB) {
      return void 0;
    }
    const res = [
      isTile ? object.x : 0,
      isTile ? object.y : 0,
      (isTile ? object.x : 0) + object.width,
      (isTile ? object.y : 0) + object.height
    ];
    return res;
  }
  function getEllipsis(object, isTile = false) {
    if (getColType(object.shape) !== COL_TYPE_CIR) {
      return void 0;
    }
    if (object.width == object.height) {
      return void 0;
    }
    const rx = object.width / 2;
    const ry = object.height / 2;
    const res = {
      x: isTile ? rx + object.x : 0,
      y: isTile ? ry + object.y : 0,
      rx,
      ry
    };
    return res;
  }
  function getCir(object, isTile = false) {
    if (getColType(object.shape) !== COL_TYPE_CIR) {
      return void 0;
    }
    if (object.width != object.height) {
      return void 0;
    }
    const r = object.width / 2;
    const res = {
      x: isTile ? r + object.x : 0,
      y: isTile ? r + object.y : 0,
      r
    };
    return res;
  }
  function getPoint(object, isTile = false) {
    if (getColType(object.shape) !== COL_TYPE_POINT) {
      return void 0;
    }
    const res = [isTile ? object.x : 0, isTile ? object.y : 0];
    return res;
  }
  function getTileObjects(object) {
    if (!object.tile) {
      return [];
    }
    const tile = object.tile;
    if (!tile.objectGroup) {
      return [];
    }
    return tile.objectGroup.objects;
  }
  function getTileCol(object) {
    const [first, second] = getTileObjects(object);
    const colType = getColType(first.shape, second == null ? void 0 : second.shape);
    if (colType === COL_TYPE_CAPSULE) {
      const res = {
        capsule: getCapsuleCol(first, second)
      };
      return res;
    } else {
      const res = getCol(first, true);
      if (object.tileFlippedHorizontally && res.aabb) {
        const col_w = res.aabb[2] - res.aabb[0];
        const rel_x = res.aabb[0];
        const flp_x = object.width - rel_x - col_w;
        res.aabb[0] = flp_x;
        res.aabb[2] = flp_x + col_w;
      }
      return res;
    }
  }
  function getCapsuleCol(a, b) {
    if (getColType(a.shape, b.shape) !== COL_TYPE_CAPSULE) {
      return void 0;
    }
    const ra = a.width / 2;
    const rb = b.width / 2;
    const cirA = {
      x: 0,
      y: 0,
      r: ra
    };
    const cirB = {
      x: b.x + rb - (a.x + ra),
      y: b.y + rb - (a.y + ra),
      r: rb
    };
    const res = [cirA, cirB];
    return res;
  }
  function getCol(object, isTile = false) {
    if (object.tile) return getTileCol(object);
    const res = {
      poly: getPolygon(object, isTile),
      aabb: getAABB(object, isTile),
      cir: getCir(object, isTile),
      ellipsis: getEllipsis(object, isTile),
      point: getPoint(object, isTile),
      capsule: void 0
    };
    return res;
  }

  // node_modules/pathe/dist/shared/pathe.ff20891b.mjs
  var _DRIVE_LETTER_START_RE = /^[A-Za-z]:\//;
  function normalizeWindowsPath(input = "") {
    if (!input) {
      return input;
    }
    return input.replace(/\\/g, "/").replace(_DRIVE_LETTER_START_RE, (r) => r.toUpperCase());
  }
  var _UNC_REGEX = /^[/\\]{2}/;
  var _IS_ABSOLUTE_RE = /^[/\\](?![/\\])|^[/\\]{2}(?!\.)|^[A-Za-z]:[/\\]/;
  var _DRIVE_LETTER_RE = /^[A-Za-z]:$/;
  var _ROOT_FOLDER_RE = /^\/([A-Za-z]:)?$/;
  var sep = "/";
  var delimiter = ":";
  var normalize = function(path2) {
    if (path2.length === 0) {
      return ".";
    }
    path2 = normalizeWindowsPath(path2);
    const isUNCPath = path2.match(_UNC_REGEX);
    const isPathAbsolute = isAbsolute(path2);
    const trailingSeparator = path2[path2.length - 1] === "/";
    path2 = normalizeString(path2, !isPathAbsolute);
    if (path2.length === 0) {
      if (isPathAbsolute) {
        return "/";
      }
      return trailingSeparator ? "./" : ".";
    }
    if (trailingSeparator) {
      path2 += "/";
    }
    if (_DRIVE_LETTER_RE.test(path2)) {
      path2 += "/";
    }
    if (isUNCPath) {
      if (!isPathAbsolute) {
        return `//./${path2}`;
      }
      return `//${path2}`;
    }
    return isPathAbsolute && !isAbsolute(path2) ? `/${path2}` : path2;
  };
  var join = function(...arguments_) {
    if (arguments_.length === 0) {
      return ".";
    }
    let joined;
    for (const argument of arguments_) {
      if (argument && argument.length > 0) {
        if (joined === void 0) {
          joined = argument;
        } else {
          joined += `/${argument}`;
        }
      }
    }
    if (joined === void 0) {
      return ".";
    }
    return normalize(joined.replace(/\/\/+/g, "/"));
  };
  function cwd() {
    if (typeof process !== "undefined" && typeof process.cwd === "function") {
      return process.cwd().replace(/\\/g, "/");
    }
    return "/";
  }
  var resolve = function(...arguments_) {
    arguments_ = arguments_.map((argument) => normalizeWindowsPath(argument));
    let resolvedPath = "";
    let resolvedAbsolute = false;
    for (let index = arguments_.length - 1; index >= -1 && !resolvedAbsolute; index--) {
      const path2 = index >= 0 ? arguments_[index] : cwd();
      if (!path2 || path2.length === 0) {
        continue;
      }
      resolvedPath = `${path2}/${resolvedPath}`;
      resolvedAbsolute = isAbsolute(path2);
    }
    resolvedPath = normalizeString(resolvedPath, !resolvedAbsolute);
    if (resolvedAbsolute && !isAbsolute(resolvedPath)) {
      return `/${resolvedPath}`;
    }
    return resolvedPath.length > 0 ? resolvedPath : ".";
  };
  function normalizeString(path2, allowAboveRoot) {
    let res = "";
    let lastSegmentLength = 0;
    let lastSlash = -1;
    let dots = 0;
    let char = null;
    for (let index = 0; index <= path2.length; ++index) {
      if (index < path2.length) {
        char = path2[index];
      } else if (char === "/") {
        break;
      } else {
        char = "/";
      }
      if (char === "/") {
        if (lastSlash === index - 1 || dots === 1) ;
        else if (dots === 2) {
          if (res.length < 2 || lastSegmentLength !== 2 || res[res.length - 1] !== "." || res[res.length - 2] !== ".") {
            if (res.length > 2) {
              const lastSlashIndex = res.lastIndexOf("/");
              if (lastSlashIndex === -1) {
                res = "";
                lastSegmentLength = 0;
              } else {
                res = res.slice(0, lastSlashIndex);
                lastSegmentLength = res.length - 1 - res.lastIndexOf("/");
              }
              lastSlash = index;
              dots = 0;
              continue;
            } else if (res.length > 0) {
              res = "";
              lastSegmentLength = 0;
              lastSlash = index;
              dots = 0;
              continue;
            }
          }
          if (allowAboveRoot) {
            res += res.length > 0 ? "/.." : "..";
            lastSegmentLength = 2;
          }
        } else {
          if (res.length > 0) {
            res += `/${path2.slice(lastSlash + 1, index)}`;
          } else {
            res = path2.slice(lastSlash + 1, index);
          }
          lastSegmentLength = index - lastSlash - 1;
        }
        lastSlash = index;
        dots = 0;
      } else if (char === "." && dots !== -1) {
        ++dots;
      } else {
        dots = -1;
      }
    }
    return res;
  }
  var isAbsolute = function(p) {
    return _IS_ABSOLUTE_RE.test(p);
  };
  var toNamespacedPath = function(p) {
    return normalizeWindowsPath(p);
  };
  var _EXTNAME_RE = /.(\.[^./]+)$/;
  var extname = function(p) {
    const match = _EXTNAME_RE.exec(normalizeWindowsPath(p));
    return match && match[1] || "";
  };
  var relative = function(from, to) {
    const _from = resolve(from).replace(_ROOT_FOLDER_RE, "$1").split("/");
    const _to = resolve(to).replace(_ROOT_FOLDER_RE, "$1").split("/");
    if (_to[0][1] === ":" && _from[0][1] === ":" && _from[0] !== _to[0]) {
      return _to.join("/");
    }
    const _fromCopy = [..._from];
    for (const segment of _fromCopy) {
      if (_to[0] !== segment) {
        break;
      }
      _from.shift();
      _to.shift();
    }
    return [..._from.map(() => ".."), ..._to].join("/");
  };
  var dirname = function(p) {
    const segments = normalizeWindowsPath(p).replace(/\/$/, "").split("/").slice(0, -1);
    if (segments.length === 1 && _DRIVE_LETTER_RE.test(segments[0])) {
      segments[0] += "/";
    }
    return segments.join("/") || (isAbsolute(p) ? "/" : ".");
  };
  var format = function(p) {
    var _a;
    const segments = [p.root, p.dir, (_a = p.base) != null ? _a : p.name + p.ext].filter(Boolean);
    return normalizeWindowsPath(
      p.root ? resolve(...segments) : segments.join("/")
    );
  };
  var basename = function(p, extension) {
    const lastSegment = normalizeWindowsPath(p).split("/").pop();
    return extension && lastSegment.endsWith(extension) ? lastSegment.slice(0, -extension.length) : lastSegment;
  };
  var parse = function(p) {
    const root = normalizeWindowsPath(p).split("/").shift() || "/";
    const base = basename(p);
    const extension = extname(base);
    return {
      root,
      dir: dirname(p),
      base,
      ext: extension,
      name: base.slice(0, base.length - extension.length)
    };
  };
  var path = {
    __proto__: null,
    basename,
    delimiter,
    dirname,
    extname,
    format,
    isAbsolute,
    join,
    normalize,
    normalizeString,
    parse,
    relative,
    resolve,
    sep,
    toNamespacedPath
  };

  // utils.ts
  function getImgPath(img) {
    const parts = img.split(path.sep);
    const srcIndex = parts.indexOf("assets");
    const res = srcIndex !== -1 ? path.join(...parts.slice(srcIndex)) : img;
    return res;
  }

  // index.ts
  function getPropByType(object, typeName) {
    const props = object.properties();
    const entry = Object.entries(props).find(([_, value]) => {
      const prop = value;
      if (prop.typeName === typeName) {
        return true;
      }
    });
    if (entry == null) {
      return void 0;
    }
    const [_key, res] = entry;
    return res;
  }
  function getPos(object) {
    const collisioType = getObjectColType(object);
    const isTile = object.tile != void 0;
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
            null
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
            null
          );
        }
        return [x, y];
      }
      case COL_TYPE_NONE:
        return [object.x, object.y + (isTile ? -object.height : 0)];
    }
  }
  function getRigidBody(object, prop) {
    const value = prop.value;
    const collisionShape = getCol(object);
    const res = {
      angular_damping: value["angular_damping"],
      dynamic_friction: value["dynamic_friction"],
      flags: value["flags"].value,
      linear_damping: value["linear_damping"],
      mass: value["mass"],
      restitution: value["restitution"],
      static_friction: value["static_friction"],
      collision_shape: collisionShape
    };
    return res;
  }
  function getSensor(object, prop) {
    const value = prop.value;
    const res = {
      is_enabled: value["is_enabled"],
      collision_shape: getCol(object)
    };
    return res;
  }
  function getMover(object, prop) {
    var _a;
    const value = prop.value;
    const ref = Number((_a = value["ref"]) == null ? void 0 : _a.id) || 0;
    const res = {
      speed: value["speed"],
      ref
    };
    return res;
  }
  function getMoverPath(object, prop) {
    const value = prop.value;
    const collisionShape = getCol(object);
    const res = {
      point: collisionShape.point,
      cir: collisionShape.cir,
      aabb: collisionShape.aabb,
      ellipsis: collisionShape.ellipsis
    };
    if (collisionShape.poly) {
      let poly = collisionShape.poly;
      res.line = [0, 0, 0, 0];
      for (let i = 0; i < Math.min(poly.length, 4); i++) {
        res.line[i] = poly[i];
      }
    }
    return res;
  }
  function getSprite(object, x, y, layerProps) {
    var _a, _b;
    if (!object.tile) {
      return void 0;
    }
    const [first] = getTileObjects(object);
    const colType = getColType(first == null ? void 0 : first.shape);
    const offset = [0, 0];
    if (colType == COL_TYPE_CIR) {
      offset[0] = object.x - x;
      offset[1] = object.y - object.height - y;
    }
    const { imageFileName } = object.tile;
    const path2 = getImgPath(imageFileName);
    let flip = 0;
    flip = flip | (object.tileFlippedHorizontally ? 1 : 0);
    flip = flip | (object.tileFlippedVertically ? 2 : 0);
    const layer = ((_a = layerProps.sprite_layer) == null ? void 0 : _a.layer) ? layerProps.sprite_layer.layer : 0;
    const y_sort = ((_b = layerProps.sprite_layer) == null ? void 0 : _b.y_sort) ? layerProps.sprite_layer.y_sort : false;
    const res = {
      path: path2,
      flip,
      offset,
      layer,
      y_sort
    };
    return res;
  }
  function getReactiveImpulse(_object, prop) {
    const value = prop.value;
    const res = {
      cooldown: value["cooldown"],
      magnitude: value["magnitude"],
      normalize: value["normalize"]
    };
    return res;
  }
  function getReactiveSpriteOffset(_object, prop) {
    var _a;
    const value = prop.value;
    const ref = Number((_a = value["ref"]) == null ? void 0 : _a.id) || 0;
    const res = {
      delay: value["delay"],
      magnitude: value["magnitude"],
      ref
    };
    return res;
  }
  function getReactiveAnimation(_object, prop) {
    const value = prop.value;
    const res = {
      animation_index: value["animation_index"]
    };
    return res;
  }
  function getPlunger(_object, prop) {
    const value = prop.value;
    const res = {
      charge_force_max: value["charge_force_max"],
      charge_force_min: value["charge_force_min"],
      release_force_max: value["release_force_max"],
      release_force_min: value["release_force_min"]
    };
    return res;
  }
  function getChargedImpulse(_object, prop) {
    const value = prop.value;
    const res = {
      angle_degrees: value["angle_degrees"],
      magnitude: value["magnitude"],
      charge_speed: value["speed_charge"],
      release_speed: value["speed_release"],
      reset_target: value["reset_target"],
      auto_shoot: value["auto_shoot"],
      auto_shoot_hold: value["auto_shoot_hold"]
    };
    return res;
  }
  function getSpinner(_object, prop) {
    const value = prop.value;
    const res = {
      damping: value["damping"],
      spin_force: value["spin_force"],
      stop_threshold: value["stop_threshold"]
    };
    return res;
  }
  function getBucket(_object, prop) {
    const value = prop.value;
    const res = {
      animation_shoot: value["animation_shoot"],
      animation_on: value["animation_on"],
      animation_off: value["animation_off"],
      impulse_angle_degrees: value["impulse_angle_degrees"],
      impulse_magnitude: value["impulse_magnitude"],
      delay: value["delay"]
    };
    return res;
  }
  function getTableSwitcher(_object, prop) {
    const value = prop.value;
    const res = {
      table: value["table"].value
    };
    return res;
  }
  function getFlip(_object, prop) {
    const value = prop.value;
    const res = {
      type: value["type"].value,
      is_enabled: value["is_enabled"]
    };
    return res;
  }
  function getFlipper(_object, prop) {
    const value = prop.value;
    const res = {
      velocity_easing_function: value["velocity_easing_function"].value,
      velocity_scale: value["velocity_scale"],
      velocity_radius_min: value["velocity_radius_min"],
      velocity_radius_max: value["velocity_radius_max"]
    };
    return res;
  }
  function getSpriteOffset(_object, prop) {
    const value = prop.value;
    const res = [value["x"], value["y"]];
    return res;
  }
  function getGravity(_object, prop) {
    const value = prop.value;
    const res = {
      value: value["value"]
    };
    return res;
  }
  function getCounter(_object, prop) {
    const value = prop.value;
    const res = {
      type: value["type"].value,
      max: value["max"],
      min: value["min"],
      resolution: value["resolution"],
      value: value["value"],
      value_initial: value["value"]
    };
    return res;
  }
  function getBallSaver(_object, prop) {
    const value = prop.value;
    const res = {
      duration: value["duration"],
      save_delay: value["save_delay"],
      is_enabled: value["is_enabled"]
    };
    return res;
  }
  function getCrankAnimation(_object, prop) {
    const value = prop.value;
    const res = {
      interval: value["interval"]
    };
    return res;
  }
  function getSfxSequence(_object, prop) {
    const value = prop.value;
    const res = {
      type: value["type"].value,
      vol_min: value["vol_min"],
      vol_max: value["vol_max"],
      pitch_min: value["pitch_min"],
      pitch_max: value["pitch_max"],
      reset_time: value["reset_time"],
      clips: value["clips"].split("\n")
    };
    return res;
  }
  function getMessage(_object, prop) {
    const value = prop.value;
    const res = {
      sequence_type: value["sequence_type"].value,
      sequence_reset_time: value["sequence_reset_time"],
      hide_time: value["hide_time"],
      text: value["text"].split("\n")
    };
    return res;
  }
  function getSfxToAnimation(_object, prop) {
    const value = prop.value;
    const res = {
      animation: value["animation"],
      sfx: value["sfx"]
    };
    return res;
  }
  function getAnimator(_object, prop) {
    const value = prop.value;
    const res = {
      play_on_start: value["play_on_start"],
      initial_animation: value["initial_animation"],
      transitions: []
    };
    return res;
  }
  function getAnimatorTransition(_object, prop) {
    const value = prop.value;
    const res = [value["from"], value["to"]];
    return res;
  }
  function getScoreFxOffset(_object, prop) {
    const value = prop.value;
    const res = [value["x"], value["y"]];
    return res;
  }
  function getSwitchValue(_object, prop) {
    const value = prop.value;
    const res = {
      is_enabled: value["is_enabled"],
      value: value["value"],
      animation_on: value["animation_on"],
      animation_off: value["animation_off"]
    };
    return res;
  }
  function getEntityList(_object, prop) {
    const value = prop.value;
    const res = {
      prev: Number(value["prev"].id),
      next: Number(value["next"].id)
    };
    return res;
  }
  function getReset(_object, prop) {
    const value = prop.value;
    const res = {
      flags: value["flags"].value
    };
    return res;
  }
  function getForceField(_object, prop) {
    const value = prop.value;
    const res = {
      angle_degrees: value["angle_degrees"],
      magnitude: value["magnitude"],
      is_enabled: value["is_enabled"]
    };
    return res;
  }
  function getAttractor(_object, prop) {
    const value = prop.value;
    const res = {
      flags: value["flags"].value,
      offset: [value["offset_x"], value["offset_y"]],
      radius: value["radius"],
      force: value["force"],
      damping: value["damping"],
      distance_threshold: value["distance_threshold"]
    };
    return res;
  }
  function getAction(object, key, prop) {
    var _a;
    const value = prop.value;
    const action_ref = Number(value["action_ref"].id) || object.id;
    const res = {
      _name: key,
      debug: value["debug"],
      action_argument: value["action_argument"],
      action_ref,
      action_type: value["action_type"].value,
      action_delay: value["action_delay"],
      action_cooldown: value["action_cooldown"],
      event_condition: value["event_condition"],
      event_condition_type: (_a = value["event_condition_type"]) == null ? void 0 : _a.value,
      event_type: value["event_type"].value
    };
    return res;
  }
  function getSpriteLayer(prop) {
    const value = prop.value;
    const res = {
      layer: value["layer"].value,
      y_sort: value["y_sort"],
      y_sort_offset: value["y_sort_offset"]
    };
    return res;
  }
  function getCollisionLayer(prop) {
    const value = prop.value;
    const res = {
      layer: value["layer"].value
    };
    return res;
  }
  function getSpawner(prop) {
    var _a;
    const value = prop.value;
    const ref = Number((_a = value["ref"]) == null ? void 0 : _a.id) || 0;
    const res = {
      ref,
      type: value["type"].value,
      zones: []
    };
    return res;
  }
  function getSpawnZoneRef(prop) {
    var _a;
    const value = prop.value;
    const ref = Number((_a = value["ref"]) == null ? void 0 : _a.id) || 0;
    const res = {
      ref
    };
    return res;
  }
  function getSpawnZone(object, prop) {
    const value = prop.value;
    const collisionShape = getCol(object);
    const res = {
      mode: value["mode"].value,
      capacity: value["capacity"],
      point: collisionShape.point,
      cir: collisionShape.cir,
      aabb: collisionShape.aabb
    };
    return res;
  }
  function handleObjectLayer(layer, layer_index) {
    const res = [];
    if (!layer.isObjectLayer) {
      return res;
    }
    const layerProps = Object.entries(layer.properties()).reduce(
      (acc, [_key, value]) => {
        const prop = value;
        switch (prop.typeName) {
          case "sprite_layer": {
            return __spreadProps(__spreadValues({}, acc), {
              sprite_layer: getSpriteLayer(prop)
            });
          }
          case "collision_layer": {
            return __spreadProps(__spreadValues({}, acc), {
              collision_layer: getCollisionLayer(prop)
            });
          }
        }
        return acc;
      },
      {}
    );
    const objectGroup = layer;
    return objectGroup.objects.map((item) => {
      var _a, _b;
      const [x, y] = getPos(item);
      const res2 = Object.entries(item.properties()).reduce(
        (acc, [key, value]) => {
          const prop = value;
          switch (prop.typeName) {
            case "entity_flags": {
              return __spreadProps(__spreadValues({}, acc), {
                flags: prop.value
              });
            }
            case "rigid_body":
              return __spreadProps(__spreadValues({}, acc), {
                rigid_body: getRigidBody(item, prop)
              });
            case "plunger":
              return __spreadProps(__spreadValues({}, acc), {
                plunger: getPlunger(item, prop)
              });
            case "charged_impulse":
              return __spreadProps(__spreadValues({}, acc), {
                charged_impulse: getChargedImpulse(item, prop)
              });
            case "spinner":
              return __spreadProps(__spreadValues({}, acc), {
                spinner: getSpinner(item, prop)
              });
            case "bucket":
              return __spreadProps(__spreadValues({}, acc), {
                bucket: getBucket(item, prop)
              });
            case "table_switcher":
              return __spreadProps(__spreadValues({}, acc), {
                table_switcher: getTableSwitcher(item, prop)
              });
            case "flipper":
              return __spreadProps(__spreadValues({}, acc), {
                flipper: getFlipper(item, prop)
              });
            case "flip":
              return __spreadProps(__spreadValues({}, acc), {
                flip: getFlip(item, prop)
              });
            case "reactive_impulse":
              return __spreadProps(__spreadValues({}, acc), {
                reactive_impulse: getReactiveImpulse(item, prop)
              });
            case "reactive_sprite_offset":
              return __spreadProps(__spreadValues({}, acc), {
                reactive_sprite_offset: getReactiveSpriteOffset(item, prop)
              });
            case "reactive_animation":
              return __spreadProps(__spreadValues({}, acc), {
                reactive_animation: getReactiveAnimation(item, prop)
              });
            case "sprite_offset":
              if (acc.spr != null) {
                const spr = __spreadProps(__spreadValues({}, acc.spr), {
                  offset: getSpriteOffset(item, prop)
                });
                return __spreadProps(__spreadValues({}, acc), {
                  spr
                });
              }
              return acc;
            case "sprite_layer":
              const sprite_layer = getSpriteLayer(prop);
              if (acc.spr != null) {
                const spr = __spreadValues(__spreadValues({}, acc.spr), sprite_layer);
                return __spreadProps(__spreadValues({}, acc), {
                  spr
                });
              }
              return acc;
            case "collision_layer":
              return __spreadProps(__spreadValues({}, acc), {
                collision_layer: getCollisionLayer(prop)
              });
            case "gravity":
              return __spreadProps(__spreadValues({}, acc), {
                gravity: getGravity(item, prop)
              });
            case "counter":
              return __spreadProps(__spreadValues({}, acc), {
                counter: getCounter(item, prop)
              });
            case "ball_saver":
              return __spreadProps(__spreadValues({}, acc), {
                ball_saver: getBallSaver(item, prop)
              });
            case "crank_animation":
              return __spreadProps(__spreadValues({}, acc), {
                crank_animation: getCrankAnimation(item, prop)
              });
            case "sensor":
              return __spreadProps(__spreadValues({}, acc), {
                sensor: getSensor(item, prop)
              });
            case "mover":
              return __spreadProps(__spreadValues({}, acc), {
                mover: getMover(item, prop)
              });
            case "mover_path":
              return __spreadProps(__spreadValues({}, acc), {
                mover_path: getMoverPath(item, prop)
              });
            case "switch_value":
              return __spreadProps(__spreadValues({}, acc), {
                switch_value: getSwitchValue(item, prop)
              });
            case "entity_list":
              return __spreadProps(__spreadValues({}, acc), {
                entity_list: getEntityList(item, prop)
              });
            case "reset":
              return __spreadProps(__spreadValues({}, acc), {
                reset: getReset(item, prop)
              });
            case "force_field":
              return __spreadProps(__spreadValues({}, acc), {
                force_field: getForceField(item, prop)
              });
            case "attractor":
              return __spreadProps(__spreadValues({}, acc), {
                attractor: getAttractor(item, prop)
              });
            case "sfx_sequence":
              if (acc.sfx_sequences == null) {
                acc.sfx_sequences = [];
              }
              return __spreadProps(__spreadValues({}, acc), {
                sfx_sequences: [
                  ...acc.sfx_sequences,
                  getSfxSequence(item, prop)
                ]
              });
            case "message":
              if (acc.messages == null) {
                acc.messages = [];
              }
              return __spreadProps(__spreadValues({}, acc), {
                messages: [...acc.messages, getMessage(item, prop)]
              });
            case "sfx_to_animation":
              if (acc.sfx_to_animation == null) {
                acc.sfx_to_animation = [];
              }
              return __spreadProps(__spreadValues({}, acc), {
                sfx_to_animation: [
                  ...acc.sfx_to_animation,
                  getSfxToAnimation(item, prop)
                ]
              });
            case "action":
              if (acc.actions == null) {
                acc.actions = [];
              }
              return __spreadProps(__spreadValues({}, acc), {
                actions: [...acc.actions, getAction(item, key, prop)]
              });
            case "animation":
              if (acc.animator == null) {
                return __spreadProps(__spreadValues({}, acc), {
                  animator: {
                    play_on_start: false,
                    initial_animation: 1,
                    transitions: []
                  }
                });
              } else {
                return acc;
              }
            case "animator_transition":
              if (acc.animator == null) {
                acc.animator = {
                  play_on_start: false,
                  initial_animation: 1,
                  transitions: []
                };
              }
              return __spreadProps(__spreadValues({}, acc), {
                animator: __spreadProps(__spreadValues({}, acc.animator), {
                  transitions: [
                    ...acc.animator.transitions,
                    getAnimatorTransition(item, prop)
                  ]
                })
              });
            case "animator":
              if (acc.animator == null) {
                return __spreadProps(__spreadValues({}, acc), {
                  animator: getAnimator(item, prop)
                });
              } else {
                return __spreadProps(__spreadValues({}, acc), {
                  animator: __spreadProps(__spreadValues({}, getAnimator(item, prop)), {
                    transitions: [...acc.animator.transitions]
                  })
                });
              }
            case "score_fx_offset":
              return __spreadProps(__spreadValues({}, acc), {
                score_fx_offset: getScoreFxOffset(item, prop)
              });
            case "spawn_zone_ref":
              const zoneRef = getSpawnZoneRef(prop);
              if (acc.spawner == null) {
                const spawner2 = {
                  ref: 0,
                  type: 0,
                  zones: [zoneRef.ref]
                };
                return __spreadProps(__spreadValues({}, acc), {
                  spawner: spawner2
                });
              } else {
                return __spreadProps(__spreadValues({}, acc), {
                  spawner: __spreadProps(__spreadValues({}, acc.spawner), {
                    zones: [...acc.spawner.zones, zoneRef.ref]
                  })
                });
              }
            case "spawner":
              const spawner = getSpawner(prop);
              if (acc.spawner == null) {
                return __spreadProps(__spreadValues({}, acc), {
                  spawner
                });
              } else {
                return __spreadProps(__spreadValues({}, acc), {
                  spawner: __spreadProps(__spreadValues({}, spawner), {
                    zones: [...acc.spawner.zones, ...spawner.zones]
                  })
                });
              }
            case "spawn_zone":
              return __spreadProps(__spreadValues({}, acc), {
                spawn_zone: getSpawnZone(item, prop)
              });
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
          collision_layer: layerProps.collision_layer
        }
      );
      if (res2 == null) {
        return null;
      }
      if (res2.spr != null) {
        if (((_b = (_a = res2.rigid_body) == null ? void 0 : _a.collision_shape) == null ? void 0 : _b.poly) != null) {
          if (res2.spr.flip != 0) {
            res2.rigid_body.collision_shape.poly = flipPolygon(
              res2.rigid_body.collision_shape.poly,
              res2.spr.flip,
              item.width,
              item.height
            );
          }
        }
      }
      return res2;
    }).filter(Boolean);
  }
  function getBgTexPath(prop) {
    const value = prop.value;
    const res = getImgPath(value["image"].localFile);
    return res;
  }
  function getFlippersProps(prop) {
    const value = prop.value;
    const res = {
      flip_velocity: value["flip_velocity"],
      release_velocity: value["release_velocity"],
      rotation_max_degrees: value["rotation_max_degrees"],
      rotation_min_degrees: value["rotation_min_degrees"]
    };
    return res;
  }
  function getPhysicsProps(prop) {
    const value = prop.value;
    const res = {
      steps: Math.floor(value["steps"]),
      max_rotation: value["max_rotation"],
      max_translation: value["max_translation"],
      penetration_allowance: value["penetration_allowance"],
      penetration_correction: value["penetration_correction"]
    };
    return res;
  }
  function getTablePropsProps(prop) {
    const value = prop.value;
    const res = {
      balls: value["balls"],
      balls_max: value["balls_max"],
      score: value["score"],
      score_mult: value["score_mult"],
      score_mult_max: value["score_mult_max"]
    };
    return res;
  }
  function getTableProps(map) {
    const res = Object.entries(map.properties()).reduce(
      (acc, [_key, value]) => {
        const prop = value;
        switch (prop.typeName) {
          case "bg_image":
            return __spreadProps(__spreadValues({}, acc), {
              bg_tex_path: getBgTexPath(prop)
            });
          case "table_props": {
            return __spreadValues(__spreadValues({}, acc), getTablePropsProps(prop));
          }
          case "flippers_props":
            return __spreadProps(__spreadValues({}, acc), {
              flippers_props: getFlippersProps(prop)
            });
          case "physics_props": {
            return __spreadProps(__spreadValues({}, acc), {
              physics_props: getPhysicsProps(prop)
            });
          }
        }
        return acc;
      },
      {}
    );
    return res;
  }
  var pinbJSONFormat = {
    name: "Pinball JSON table",
    extension: "pinbjson",
    write: function(map, fileName) {
      const props = getTableProps(map);
      const m = {
        version: 1,
        props,
        entities_count: 0,
        entities_max_id: 0,
        entities: []
      };
      for (let i = 0; i < map.layerCount; ++i) {
        const layer = map.layerAt(i);
        if (layer.isObjectLayer) {
          const entities = handleObjectLayer(layer, i);
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
      return void 0;
    }
  };
  tiled.registerMapFormat("pinbjson", pinbJSONFormat);
})();
