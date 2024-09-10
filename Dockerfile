FROM registry.gitlab.steamos.cloud/steamrt/scout/sdk:latest

WORKDIR /app

COPY src/ ./src/
COPY luna/ ./luna/
COPY platforms/linux ./platforms/linux/
COPY Makefile ./
