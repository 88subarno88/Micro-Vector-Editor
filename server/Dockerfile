# syntax=docker/dockerfile:1.7
#
# The server's three images. Build from the repository root, which holds the core and the vcpkg manifest:
#
#   docker build -f server/Dockerfile --target api -t vec-api .        (the default target)
#   docker build -f server/Dockerfile --target worker -t vec-worker .
#   docker build -f server/Dockerfile --target migrate -t vec-migrate .
#
# They read their settings from the environment. The API needs DATABASE_URL, REDIS_URL and JWT_SECRET (see
# common/config/Config.h); the worker and migrate need only DATABASE_URL. The API and the worker share
# exported files in EXPORT_DIR, /var/lib/vec/exports: mount the same volume there in both.

# ---- Build: the C++ libraries from vcpkg, then the server, in Release. ----
# Debian 13, for GCC 14. Debian 12's GCC 12 miscompiles the server's coroutines: logging in crashed it with
# a double free, while the same code built with GCC 13 or 14 runs fine.
FROM debian:trixie AS build

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake ninja-build pkg-config git curl ca-certificates zip unzip tar jq python3 \
        bison flex autoconf autoconf-archive automake libtool linux-libc-dev \
    && rm -rf /var/lib/apt/lists/*

# vcpkg at the manifest's baseline, so the libraries are the versions every other build uses.
WORKDIR /src
COPY vcpkg.json ./
RUN git clone https://github.com/microsoft/vcpkg.git /opt/vcpkg \
    && git -C /opt/vcpkg checkout "$(jq -r '."builtin-baseline"' vcpkg.json)" \
    && /opt/vcpkg/bootstrap-vcpkg.sh -disableMetrics

# The libraries on their own, before the sources are copied in: changing the server's code doesn't rebuild
# them. vcpkg's binary cache outlives the build, so changing vcpkg.json only rebuilds what changed.
RUN --mount=type=cache,target=/root/.cache/vcpkg \
    /opt/vcpkg/vcpkg install --x-manifest-root=. --x-feature=server --x-install-root=/opt/vcpkg_installed \
        --triplet=x64-linux

COPY CMakeLists.txt ./
COPY core core
COPY render render
COPY server server

# The build folder is cached too, so a change to one file recompiles only what depends on it. The binary
# is copied out of it, since a cache mount isn't part of the image.
RUN --mount=type=cache,target=/src/build \
    cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake \
        -DVCPKG_MANIFEST_INSTALL=OFF -DVCPKG_INSTALLED_DIR=/opt/vcpkg_installed -DVCPKG_TARGET_TRIPLET=x64-linux \
        -DBUILD_SERVER=ON -DBUILD_DESKTOP=OFF -DBUILD_CLI=OFF -DBUILD_TESTS=OFF \
    && cmake --build build --target vec_api vec_worker \
    && mkdir -p /out && cp build/vec_api build/vec_worker /out/ && strip /out/vec_api /out/vec_worker

# ---- What both servers run on. vcpkg links their libraries statically, so they need only the C and C++
# runtimes, from the same Debian release they were built on. ----
FROM debian:trixie-slim AS runtime

# Root certificates, for a database or Redis reached over TLS. The exports folder belongs to the user the
# servers run as: a new volume mounted there takes that over, so the worker can write to it.
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates \
    && rm -rf /var/lib/apt/lists/* \
    && useradd --system --uid 10001 --no-create-home --shell /usr/sbin/nologin vec \
    && mkdir -p /var/lib/vec/exports && chown vec:vec /var/lib/vec/exports

ENV EXPORT_DIR=/var/lib/vec/exports
USER vec

# ---- The export worker. ----
FROM runtime AS worker

# A font for text in PNGs: without one the renderer refuses rather than leave text out (render/src/Png.cpp).
USER root
RUN apt-get update && apt-get install -y --no-install-recommends fonts-liberation \
    && rm -rf /var/lib/apt/lists/*
USER vec

COPY --from=build /out/vec_worker /usr/local/bin/vec_worker
ENTRYPOINT ["vec_worker"]

# ---- Database migrations: dbmate with the server's migrations, run to completion and gone. ----
FROM ghcr.io/amacneil/dbmate:2.37.0 AS migrate

COPY server/migrations /db/migrations
ENV DBMATE_MIGRATIONS_DIR=/db/migrations DBMATE_NO_DUMP_SCHEMA=true
# Waits for the database to come up, then applies whatever migrations it hasn't had. Needs DATABASE_URL.
CMD ["--wait", "up"]

# ---- The API server. Last, so that it's what a build without --target makes. ----
FROM runtime AS api

COPY --from=build /out/vec_api /usr/local/bin/vec_api
ENV PORT=8080
EXPOSE 8080
ENTRYPOINT ["vec_api"]
