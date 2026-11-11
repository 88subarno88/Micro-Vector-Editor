# syntax=docker/dockerfile:1.7
#
# The web app, served by nginx, which also passes /api and WebSockets on to two API servers, api-1 and
# api-2 (see nginx.conf). Build from the repository root: the app needs the core, compiled to WebAssembly.
#
#   docker build -f web/Dockerfile -t vec-web .

# ---- The core, compiled to WebAssembly, as the "wasm" preset does in CI. ----
FROM emscripten/emsdk:6.0.13 AS core

RUN apt-get update && apt-get install -y --no-install-recommends git curl zip unzip tar jq pkg-config cmake make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY vcpkg.json CMakePresets.json ./
RUN git clone https://github.com/microsoft/vcpkg.git /opt/vcpkg \
    && git -C /opt/vcpkg checkout "$(jq -r '."builtin-baseline"' vcpkg.json)" \
    && /opt/vcpkg/bootstrap-vcpkg.sh -disableMetrics
ENV VCPKG_ROOT=/opt/vcpkg

COPY CMakeLists.txt ./
COPY core core
COPY render render
RUN --mount=type=cache,target=/root/.cache \
    cmake --preset wasm && cmake --build --preset wasm --target vec_wasm \
    && mkdir -p /out && cp build/wasm/vec.js build/wasm/vec.wasm build/wasm/vec.d.ts /out/

# ---- The app, built for production with the core copied in. ----
FROM node:24.19.0-slim AS app

WORKDIR /web
COPY web/package.json web/package-lock.json ./
RUN --mount=type=cache,target=/root/.npm npm ci
COPY web ./
COPY --from=core /out/ src/wasm/
RUN npm run build

# ---- Served by nginx. ----
FROM nginx:1.29.3-alpine

COPY web/nginx.conf /etc/nginx/conf.d/default.conf
COPY --from=app /web/dist /usr/share/nginx/html
