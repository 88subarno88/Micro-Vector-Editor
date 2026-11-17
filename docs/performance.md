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

## Serving capacity

How much the whole stack (`make up`: nginx, two API servers, Postgres, Redis) can take, measured with the
load generator in [tools/loadtest](../tools/loadtest). It signs users in, shares documents among them, and
drives the real WebSocket protocol through nginx: each editor adds one rectangle, then moves it, so edits
never conflict, and every member can also move their pointer.

- **Ack latency:** from sending an op to its `ack`. **Fan-out latency:** from sending an op to another member
  receiving it.
- **Machine:** the same i7-13620H (16 threads, 16 GB). Release images from `server/Dockerfile`. The load
  generator is one Node process on the same machine, so the busiest runs share the CPU with it; its own CPU
  use is noted where it matters.

### Edits on one document

Editors each sending 25 edits a second (the client's own pace; the server allows 30).

| Editors | Offered | Acked | Refused (`busy`) | Ack p50 / p99 |
|---|---|---|---|---|
| 20 | 498/s | 498/s | 0 | 11.7 / 30.6 ms |
| 30 | 748/s | 744/s | 70 | 573 / 980 ms |
| 40 | 996/s | 687/s | 6,184 | 1,129 / 1,316 ms |
| 80 | 1,990/s | 597/s | 27,864 | 1,583 / 2,276 ms |

**One document takes about 700–750 edits a second**, about 30 people dragging at once. Neither Postgres
(~50% of a core) nor the API servers (~1 core each) is saturated: the limit is that a room runs one op at a
time, each a transaction of several round trips, and both servers take the same document's row lock in
turn. Beyond it, throughput falls rather than levelling off, as refused ops still cost work.

### Edits across many documents

Five editors per document, 25 edits a second each.

| Documents | Offered | 4 DB connections per server (before) | 16 DB connections per server (now) |
|---|---|---|---|
| 40 | ~5,000/s | all acked, p50 14 ms, p99 93 ms | all acked, p50 4.9 ms, p99 26 ms |
| 80 | ~10,000/s | 6,241/s acked, 30,458 refused, p50 4.5 s | all 9,690/s acked, p50 37 ms, p99 3.0 s |
| 120 | ~12,000/s | – | 11,500/s acked, 1,046 refused (load generator at 94% CPU) |

**With 4 connections, the system took about 5,000 edits a second.** Each op holds one of a server's 4 database
connections for its whole transaction, which caps each server near 2 cores however many it has. With 16,
now the default, it takes about 10,000 a second.

### Pointers in one document

N members each moving their pointer 10 times a second, no edits.

| Members | Presence sent | Per member |
|---|---|---|
| 10 | 1.1 MB/s | 110 KB/s |
| 25 | 17 MB/s | 680 KB/s |
| 50 | 134 MB/s | 2.7 MB/s |
| 75 | 258 MB/s (servers saturated) | 3.4 MB/s |

Every pointer move makes every server read the whole presence hash and send all of it to every member, so the
bytes grow with the cube of the members: from 10 to 50 members (5×) they grew 122× (5³ = 125). Past about 25
members in one document, presence costs more than anything else. Sending only the pointer that moved, or one
batched update per room every 100 ms, would make it linear.

### Connections

Members joined and idle (pinging every 15 s).

| Connections | Result |
|---|---|
| 2,000 (Docker's default limit, before) | **fails**: each API server stops accepting at ~1,000 sockets |
| 2,000 / 5,000 (`nofile` raised to 65,536, now) | all joined, join p50 8–10 ms, ~49 MB and under 1% CPU per server at 5,000 |

The API containers inherit an open-file limit of 1,024. Past it, `accept` fails with `EMFILE`, and Drogon's
acceptor retries in a tight loop: each server spins at ~2 cores, nginx marks both down, and every request,
logins included, gets a 502 until the connections close. About 2,000 idle WebSockets take the whole site
down. An idle connection costs about 14 KB. The next limit is nginx's: 1,024 connections per worker, two per
proxied WebSocket.

### Logins

Password hashing runs on 2 threads per server. Signing up and logging in 5,000 users (10,000 hashes) took
231 s: **about 43 hashes a second for the whole system**, so about 43 logins (or signups) a second, at
~2 cores per server while it lasts.

### Latency under light load: Nagle's algorithm

With one editor, ack latency was 2.7 ms; with five, 40.5 ms; with ten, 39.6 ms. Drogon doesn't set
`TCP_NODELAY`, so an ack written while an earlier broadcast on the same connection is still unacknowledged
waits for nginx's delayed ACK, 40 ms on Linux. Setting it in the accept callback took five editors to
3.8 ms p50 and ten to 4.0 ms. All the numbers above except the first table's 4-connection rows are with it set.

### Fixes made from these

| Fix, now in the code | Effect, re-measured on the fixed build |
|---|---|
| `TCP_NODELAY` on accepted sockets (`App.cpp`, `setAfterAcceptSockOptCallback`) | 5 editors: ack p50 40.5 → 3.6 ms; 10 editors: 39.6 → 5.4 ms |
| `ulimits: nofile: 65536` on the API services in Compose | 2,000 connections took the site down → 5,000 join (p50 7 ms), none dropped, ~50 MB per server |
| More database connections per server (`databaseConnections` in `main.cpp`, 4 → 16) | 80 documents: 6,241/s acked and 30,458 refused → all 9,552/s acked, p50 10 ms, p99 107 ms |
| Still to do: presence as deltas, or batched per room | the cube would become linear |

### How to reproduce

1. Start a separate stack so the test's users don't mix with real ones, with the auth rate limit raised (the
   test signs every user up from one address): in `deploy/.env` set `WEB_PORT=8100` and
   `AUTH_REQUESTS_PER_MINUTE=100000`, then
   `docker compose -p vecload -f deploy/docker-compose.yml --env-file deploy/.env up -d --wait`.
2. Run a scenario, for example the one-document test:
   `tools/loadtest/run.sh A-e20 --docs 1 --editors 20 --opRate 25 --seconds 20 | python3 tools/loadtest/summarize.py`.
   The others: `--docs 40 --editors 5 --opRate 25`; `--docs 1 --editors 1 --viewers 49 --opRate 0 --cursorRate 10`;
   `--docs 100 --editors 1 --viewers 49 --opRate 0`.
3. `docker compose -p vecload -f deploy/docker-compose.yml --env-file deploy/.env down -v` removes it all.
