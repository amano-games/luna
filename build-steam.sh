#!/usr/bin/env bash
set -eu

PROJECT_NAME=pinball
DOCKERFILE=Dockerfile

podman build . -t $PROJECT_NAME -f $DOCKERFILE
id=$(podman create $PROJECT_NAME)
rm -rf ./build
podman run -d --name "$id" make CC='./usr/bin/gcc-9' DEBUG=0 linux_build
podman cp "$id":/build ./build
podman rm -v "$id"
make linux_publish
