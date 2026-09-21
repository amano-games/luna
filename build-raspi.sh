#!/usr/bin/env bash
set -eu

# Build the Raspberrypi binary inside the armv6 SDK image. (Compatible with raspi zero w and 2B)
# Usage: ./luna/build-raspi.sh <repo-root> <make-args...>
# Does not wipe build/ (sysroot lives there).

ROOT_DIR="$1"
shift

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
DOCKERFILE="$SCRIPT_DIR/Dockerfile.raspi"
PROJECT_NAME=pinball-raspi

cd "$ROOT_DIR"

podman build -t "$PROJECT_NAME" -f "$DOCKERFILE" .

mkdir -p "$ROOT_DIR/build"

podman run --rm \
  -v "$ROOT_DIR/build:/app/build" \
  "$PROJECT_NAME" \
  "$@"
