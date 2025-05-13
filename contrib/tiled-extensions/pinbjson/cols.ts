/// <reference types="@mapeditor/tiled-api" />

import {
  AABB,
  Capsule,
  Cir,
  COL_TYPE_AABB,
  COL_TYPE_CAPSULE,
  COL_TYPE_CIR,
  COL_TYPE_POINT,
  COL_TYPE_POLY,
  CollisionShape,
  Poly,
} from "./types";

export function getColType(a: MapObjectShape, b?: MapObjectShape) {
  if (a == null) {
    return undefined;
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
  return undefined;
}

export function flipPolygon(
  arr: number[],
  flip: number,
  width: number,
  height: number,
) {
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

function getPolygon(object: MapObject, isTile = false) {
  if (getColType(object.shape) !== COL_TYPE_POLY) {
    return undefined;
  }
  const res: Poly = object.polygon.reduce((acc, curr) => {
    return [
      ...acc,
      ...[curr.x + (isTile ? object.x : 0), curr.y + (isTile ? object.y : 0)],
    ];
  }, []);
  return res;
}

function getAABB(object: MapObject, isTile = false) {
  if (getColType(object.shape) !== COL_TYPE_AABB) {
    return undefined;
  }

  const res: AABB = [
    isTile ? object.x : 0,
    isTile ? object.y : 0,
    (isTile ? object.x : 0) + object.width,
    (isTile ? object.y : 0) + object.height,
  ];
  return res;
}

function getCir(object: MapObject) {
  if (getColType(object.shape) !== COL_TYPE_CIR) {
    return undefined;
  }

  const res: Cir = {
    x: 0,
    y: 0,
    r: object.width / 2,
  };
  return res;
}

export function getTileObjects(object: MapObject) {
  if (!object.tile) {
    return [];
  }
  const tile = object.tile;
  if (!tile.objectGroup) {
    return [];
  }
  return tile.objectGroup.objects;
}

function getTileCol(object: MapObject) {
  const [first, second] = getTileObjects(object);
  const colType = getColType(first.shape, second?.shape);
  if (colType === COL_TYPE_CAPSULE) {
    const res = {
      capsule: getCapsuleCol(first, second),
    };
    return res;
  } else {
    const res = getCol(first, true);
    return res;
  }
}

export function getCapsuleCol(a: MapObject, b: MapObject) {
  if (getColType(a.shape, b.shape) !== COL_TYPE_CAPSULE) {
    return undefined;
  }

  const ra = a.width / 2;
  const rb = b.width / 2;
  const cirA: Cir = {
    x: 0,
    y: 0,
    r: ra,
  };
  const cirB: Cir = {
    x: b.x + rb - (a.x + ra),
    y: b.y + rb - (a.y + ra),
    r: rb,
  };

  const res: Capsule = [cirA, cirB];

  return res;
}

export function getCol(object: MapObject, isTile = false) {
  if (object.tile) return getTileCol(object);
  const res: CollisionShape = {
    poly: getPolygon(object, isTile),
    aabb: getAABB(object, isTile),
    cir: getCir(object),
    capsule: undefined,
  };

  return res;
}
