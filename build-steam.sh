#!/usr/bin/env bash
set -eu

PROJECT_NAME=pinball
DOCKERFILE=./luna/Dockerfile

podman build . -t $PROJECT_NAME -f $DOCKERFILE
id=$(podman create $PROJECT_NAME)
rm -rf ./build
podman run -d --name "$id" -it $PROJECT_NAME:latest make linux_build DEBUG=0
podman cp "$id":/app/build ./build
podman stop "$id"
podman rm -v "$id"
make linux_publish
