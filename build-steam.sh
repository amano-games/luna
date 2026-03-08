#!/usr/bin/env bash
set -eu

PROJECT_NAME=pinball
DOCKERFILE=./luna/Dockerfile

podman build -t "$PROJECT_NAME" -f "$DOCKERFILE" .

rm -rf build
mkdir -p build

podman run --rm \
  -v "$PWD/build:/app/build" \
  "$PROJECT_NAME" \
  "$@"
