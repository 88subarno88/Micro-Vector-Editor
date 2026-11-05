# Performance

Measurements taken while building the server, so that each optimisation can be judged against a number
instead of a guess. Each section says how it was measured; rerun the same steps to compare.

## Setup

- **Machine:** Intel Core i7-13620H (16 threads), Ubuntu 24.04
- **Server:** the `server` preset, a **debug** build, so absolute times are pessimistic
- **Database:** Postgres 16 in Docker on the same machine (`deploy/docker-compose.dev.yml`)

## Loading a document

**Document:** 5,000 ops, no snapshot. 500 `addShape` ops, then 4,500 moves and fill changes spread over
those shapes; 326 KB of op JSON in total. Each run starts a fresh server, so nothing is cached in memory.

- **Room load:** `loadDrawing` reading every op and applying it, as logged in `loaded room` (`ms`).
- **Client join:** from opening the WebSocket to receiving `joined`, measured by the client; includes the
  room load.
- **REST GET:** `GET /api/docs/{id}`, measured by `curl`.

### Before snapshots (Step 23.1)

| Run | Room load | Client join | REST GET |
|---|---|---|---|
| 1 | 109 ms | 128 ms | 51 ms |
| 2 | 91 ms | 109 ms | 52 ms |
| 3 | 115 ms | 134 ms | 48 ms |
| 4 | 95 ms | 114 ms | 69 ms |
| 5 | 94 ms | 112 ms | 51 ms |
| **Median** | **95 ms** | **114 ms** | **51 ms** |

The REST GET is fast only because it is **incomplete**: it sends at most 1,000 ops after the newest
snapshot, so with no snapshot it returned ops 1 to 1,000 of 5,000 and `opsComplete: false`. A client
can't draw this document from REST alone.
