FROM registry.gitlab.steamos.cloud/steamrt/scout/sdk:latest

WORKDIR /app

COPY src/ ./src/
COPY luna/ ./luna/
COPY linux ./linux/
COPY update_runtime.sh ./
COPY extract_runtime.sh ./
COPY Makefile* ./

# RUN make CC='/usr/bin/gcc-9' DEBUG=0 linux_build
