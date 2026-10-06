#!/usr/bin/env bash
set -eu

# Proxy make into the steamrt soldier SDK image (high-compat Linux).
# Usage: ./luna/build-linux-proxy.sh <repo-root> <make-args...>
# Steamworks is mounted when present; omit STEAM_APP_ID for non-Steam builds.

ROOT_DIR="$1"
shift

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
DOCKERFILE="$SCRIPT_DIR/Dockerfile.linux"
PROJECT_NAME=pinball-linux

STEAMWORKS_SDK_PATH="${STEAMWORKS_SDK_PATH:-$HOME/Developer/steamworks-sdk}"
CONTAINER_SDK_PATH="/root/Developer/steamworks-sdk"
STEAM_API_SO="$STEAMWORKS_SDK_PATH/redistributable_bin/linux64/libsteam_api.so"

cd "$ROOT_DIR"
ROOT_DIR="$PWD"

podman build -t "$PROJECT_NAME" -f "$DOCKERFILE" .

mkdir -p "$ROOT_DIR/build"

run_args=(
	--rm
	-v "$ROOT_DIR/build:/app/build"
)

# Mount Steamworks when available so steam makefile targets can link.
if [[ -f "$STEAM_API_SO" ]]; then
	run_args+=(
		-v "$STEAMWORKS_SDK_PATH:$CONTAINER_SDK_PATH:ro"
		-e STEAMWORKS_SDK_PATH="$CONTAINER_SDK_PATH"
	)
fi

podman run "${run_args[@]}" "$PROJECT_NAME" "$@"
