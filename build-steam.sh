#!/usr/bin/env bash
set -eu

ROOT_DIR="$1"
shift

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
DOCKERFILE="$SCRIPT_DIR/Dockerfile"

PROJECT_NAME=pinball

cd "$ROOT_DIR"

podman build -t "$PROJECT_NAME" -f "$DOCKERFILE" .

rm -rf "$ROOT_DIR/build"
mkdir -p "$ROOT_DIR/build"

podman run --rm \
  -v "$ROOT_DIR/build:/app/build" \
  "$PROJECT_NAME" \
  "$@"
