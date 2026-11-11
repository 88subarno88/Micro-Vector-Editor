// The engine with the real WebAssembly core: a simulated server and two clients make random edits to the
// same shapes, and messages arrive in random orders. Connections drop at random, losing whatever was in
// flight, and come back with a catch-up: the missed ops, or now and then the whole drawing. Once everything
// has been delivered, both clients must hold exactly the server's drawing. Needs the core copied in with `npm run wasm`; skipped without it.
import { describe, expect, it } from 'vitest'
import { SyncEngine, type Drawings } from './SyncEngine'

// Matches nothing, rather than failing to build, when the core hasn't been copied in.
const loadCore = Object.values(
  import.meta.glob<{ default: () => Promise<{ Document: new () => CoreDocument }> }>('../wasm/vec.js'),
)[0]

type CoreDocument = {
  applyOp(op: string): string
  loadJson(snapshot: string): string
  shapesJson(): string
  delete(): void
}

// A small seeded generator, so a failing run can be replayed.
function random(seed: number) {
  return () => {
    seed = (seed + 0x6d2b79f5) | 0
    let t = Math.imul(seed ^ (seed >>> 15), 1 | seed)
    t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296
  }
}

// Random edits to a few shape IDs, so clients often clash: moving what the other just deleted, adding an ID
// that already exists.
function randomOp(next: () => number): object {
  const id = `s${Math.floor(next() * 4)}`
  const style = { stroke: 'black', fill: ['white', 'red', 'none'][Math.floor(next() * 3)], strokeWidth: 1 }
  switch (Math.floor(next() * 5)) {
    case 0:
      return {
        kind: 'addShape',
        id,
        shape: {
          kind: 'rect',
          x: next() * 500,
          y: next() * 400,
          width: 10 + next() * 90,
          height: 10 + next() * 90,
          cornerRadius: 0,
          style,
        },
      }
    case 1:
      return { kind: 'deleteShape', id }
    case 2:
      return { kind: 'move', id, dx: next() * 40 - 20, dy: next() * 40 - 20 }
    case 3:
      return { kind: 'resize', id, factor: 0.8 + next() * 0.45 }
    default:
      return { kind: 'setStyle', id, fill: style.fill }
  }
}

// The core answers with JSON: an error, or not.
const errorIn = (result: string): string | null => JSON.parse(result).error?.message ?? null
const applyTo = (doc: CoreDocument, op: object) => errorIn(doc.applyOp(JSON.stringify(op)))

// A client's two drawings in the core. The local one is reset by copying the confirmed one's JSON, whose
// numbers round-trip exactly.
const drawingsOf = (confirmed: CoreDocument, local: CoreDocument): Drawings => ({
  applyConfirmed: (op) => applyTo(confirmed, op),
  applyLocal: (op) => applyTo(local, op),
  resetLocal: () => {
    local.loadJson(confirmed.shapesJson())
  },
  loadConfirmed: (snapshot) => errorIn(confirmed.loadJson(JSON.stringify(snapshot))),
})

type ToServer = { from: number; clientOpId: string; op: object }
type ToClient =
  | { kind: 'ack'; clientOpId: string; seq: number }
  | { kind: 'op'; seq: number; op: object }
  | { kind: 'reject'; clientOpId: string; reason: 'refused' | 'rate_limited' }

// Across all runs, so the test can check it really exercised each of these.
const seen = { rolledBack: 0, ownOpsCaughtUp: 0, resendsRecognised: 0, snapshots: 0 }

async function simulate(seed: number, steps: number) {
  const vec = await (await loadCore()).default()
  const next = random(seed)

  const server = new vec.Document()
  let seq = 0
  const log: { seq: number; clientOpId: string; op: object }[] = [] // every committed op
  const inbound: ToServer[] = [] // one queue into the server: the room runs ops one at a time
  const outbound: ToClient[][] = [[], []] // in order, per client, as on a WebSocket
  const online = [true, true]

  const clients = [0, 1].map((index) => {
    const confirmed = new vec.Document()
    const doc = new vec.Document() // what the user sees
    const engine = new SyncEngine(
      drawingsOf(confirmed, doc),
      {
        send: (clientOpId, _baseSeq, op) => online[index] && (inbound.push({ from: index, clientOpId, op }), true),
      },
      0,
    )
    return { confirmed, doc, engine }
  })

  const serverStep = () => {
    const message = inbound.shift()!
    const answers = outbound[message.from]
    // A resend of an op already stored: acked again with its seq, not applied twice.
    const stored = log.find((committed) => committed.clientOpId === message.clientOpId)
    if (stored)
      return (seen.resendsRecognised++, answers.push({ kind: 'ack', clientOpId: message.clientOpId, seq: stored.seq }))
    // Now and then the server turns an op away without looking at it, as when rate limited: the client
    // must then take back an edit that applied fine and may have others on top.
    if (next() < 0.05) return answers.push({ kind: 'reject', clientOpId: message.clientOpId, reason: 'rate_limited' })
    const refused = JSON.parse(server.applyOp(JSON.stringify(message.op))).error
    if (refused) return answers.push({ kind: 'reject', clientOpId: message.clientOpId, reason: 'refused' })
    seq += 1
    log.push({ seq, clientOpId: message.clientOpId, op: message.op })
    answers.push({ kind: 'ack', clientOpId: message.clientOpId, seq })
    if (online[1 - message.from]) outbound[1 - message.from].push({ kind: 'op', seq, op: message.op })
  }

  // The connection drops: what the client sent and the server hasn't read is lost, and so is everything the
  // server sent and the client hasn't read.
  const disconnect = (index: number) => {
    online[index] = false
    outbound[index].length = 0
    for (let i = inbound.length - 1; i >= 0; i--) if (inbound[i].from === index) inbound.splice(i, 1)
  }
  // Rejoining: the server sends what the client missed, its own ops included, or now and then its whole
  // drawing instead, as when the client is too far behind; then everything pending goes again.
  const reconnect = (index: number) => {
    const { engine } = clients[index]
    online[index] = true
    if (next() < 0.3) {
      seen.snapshots++
      engine.loadSnapshot(seq, JSON.parse(server.shapesJson()))
    } else {
      const missed = log.filter((c) => c.seq > engine.lastSeq)
      for (const committed of missed) {
        if (engine.pending.some((p) => p.clientOpId === committed.clientOpId)) seen.ownOpsCaughtUp++
        expect(engine.remote(committed.seq, committed.op, committed.clientOpId)).toBe('applied')
      }
      // Its own ops in the catch-up were recognised: none is still pending, which would show it twice.
      expect(engine.pending.filter((p) => missed.some((c) => c.clientOpId === p.clientOpId))).toEqual([])
    }
    engine.resend()
  }
  const clientStep = (index: number) => {
    const message = outbound[index].shift()!
    const { engine } = clients[index]
    if (message.kind === 'ack') engine.ack(message.clientOpId, message.seq)
    else if (message.kind === 'op') expect(engine.remote(message.seq, message.op)).toBe('applied')
    else {
      // Refused for not applying: the server did so after everything committed before it, which this
      // client has already received in order and rebased onto, so it must already be left out here too.
      const rejected = engine.pending.find((p) => p.clientOpId === message.clientOpId)!
      if (message.reason === 'refused') expect(rejected.applied).toBe(false)
      if (rejected.applied) seen.rolledBack++
      engine.reject(message.clientOpId)
    }
  }

  for (let step = 0; step < steps; step++) {
    const choice = next()
    if (choice < 0.4) clients[Math.floor(next() * 2)].engine.local(randomOp(next))
    else if (choice < 0.7 && inbound.length) serverStep()
    else if (choice < 0.73) {
      const index = Math.floor(next() * 2)
      if (online[index]) disconnect(index)
      else reconnect(index)
    } else {
      const ready = [0, 1].filter((i) => outbound[i].length)
      if (ready.length) clientStep(ready[Math.floor(next() * ready.length)])
    }
  }
  // Bring everyone back, and deliver everything still in flight.
  for (const index of [0, 1]) if (!online[index]) reconnect(index)
  while (inbound.length || outbound[0].length || outbound[1].length) {
    if (inbound.length) serverStep()
    else clientStep(outbound[0].length ? 0 : 1)
  }

  const expected = server.shapesJson()
  const results = clients.map(({ doc, engine }) => ({
    drawing: doc.shapesJson(),
    diverged: engine.diverged,
    lastSeq: engine.lastSeq,
  }))
  ;[server, ...clients.flatMap((c) => [c.confirmed, c.doc])].forEach((doc) => doc.delete())
  return { expected, results, committed: seq }
}

describe.skipIf(!loadCore)('rebasing with the real core', () => {
  it("leaves both clients with exactly the server's drawing, across many random runs", async () => {
    for (let seed = 1; seed <= 40; seed++) {
      const { expected, results, committed } = await simulate(seed, 300)
      for (const result of results) {
        expect(result.diverged, `seed ${seed}`).toBe(false)
        expect(result.lastSeq, `seed ${seed}`).toBe(committed)
        expect(result.drawing, `seed ${seed}`).toBe(expected)
      }
    }
    expect(seen.rolledBack).toBeGreaterThan(10)
    expect(seen.ownOpsCaughtUp).toBeGreaterThan(10)
    expect(seen.resendsRecognised).toBeGreaterThan(10)
    expect(seen.snapshots).toBeGreaterThan(10)
  }, 60_000)
})
