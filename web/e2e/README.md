# Browser tests

The whole system, end to end: the app built for production, in Chrome, against a running API server,
export worker, Postgres and Redis. Two people editing one document each get a browser of their own.

They need, from the repository root:

```sh
docker compose -f deploy/docker-compose.dev.yml up -d --wait
set -a; source deploy/dev.env; set +a
dbmate up
# Every test signs its users in, all from one address: raise the auth limit.
AUTH_REQUESTS_PER_MINUTE=1000 build/server/vec_api &
build/server/vec_worker &
```

and the core in `web/src/wasm` (`npm run wasm`). Then, in `web/`:

```sh
npx playwright test
```

Playwright builds the app and serves it with `vite preview`, which passes `/api` and `/ws` on to the server.
It uses the installed Google Chrome, so there's no browser to download.

Each test makes its own users and documents, so they can run against a server that has other data. They
compare what two browsers show by the dark pixels on the drawing's canvas (`picture` in `helpers.ts`).
