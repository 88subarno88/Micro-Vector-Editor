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

### With snapshots (Step 23.3)

The same document after Step 23.2, which snapshots every 200 ops. Opening it once saved a snapshot at
seq 5,000; then 199 more ops were added, the most a document can have before its next snapshot, so this
is the worst case: one snapshot (94 KB) plus 199 ops to replay.

| Run | Room load | Client join | REST GET |
|---|---|---|---|
| 1 | 19 ms | 64 ms | 31 ms |
| 2 | 20 ms | 38 ms | 33 ms |
| 3 | 29 ms | 58 ms | 49 ms |
| 4 | 32 ms | 58 ms | 52 ms |
| 5 | 19 ms | 37 ms | 35 ms |
| **Median** | **20 ms** | **58 ms** | **35 ms** |

Every run ended at the same drawing as before: lastSeq 5,199 and 500 shapes.

### Comparison

| | Before | After | Change |
|---|---|---|---|
| Room load | 95 ms | 20 ms | **4.75× faster** |
| Client join | 114 ms | 58 ms | 2× faster |
| REST GET | 51 ms, incomplete | 35 ms, complete | now usable |

- **Load time no longer grows with history.** Without snapshots it grows with every op a document has
  ever had; with them, a load reads one snapshot and at most 199 ops, whatever the document's age.
- **The client join is now mostly not the load.** The other ~40 ms is the role check and sending the
  whole drawing (94 KB) in the `joined` reply, because the client joined with `lastSeq: 0`.
- **REST GET now returns the whole document:** the snapshot plus 199 ops, with `opsComplete: true`.

### How to reproduce

1. Create a document and insert ops straight into `operations` (and set `documents.head_seq`), as the
   benchmark did with `generate_series`.
2. For each run, start a fresh `vec_api`, connect a WebSocket to `/ws/docs/{id}` and send
   `{"type":"join","lastSeq":0}`, then `GET /api/docs/{id}`.
3. Read `ms` from the server's `loaded room` log line, and time the join and GET on the client.
