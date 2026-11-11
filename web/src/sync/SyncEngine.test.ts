import { describe, expect, it } from 'vitest'
import { SyncEngine, type Transport } from './SyncEngine'
import { counters } from './testing'

// A connection that records what it sends, and can be taken down.
function wire() {
  const sent: { clientOpId: string; baseSeq: number; op: object }[] = []
  const transport = {
    up: true,
    sent,
    send(clientOpId: string, baseSeq: number, op: object) {
      if (!transport.up) return false
      sent.push({ clientOpId, baseSeq, op })
      return true
    },
  } satisfies Transport & Record<string, unknown>
  return transport
}

const ids = () => {
  let n = 0
  return () => `op-${++n}`
}

describe('a local edit', () => {
  it('shows at once, waits for the server, and is sent from the last seq the client knows', () => {
    const drawings = counters()
    const transport = wire()
    const engine = new SyncEngine(drawings, transport, 7, ids())

    expect(engine.local({ add: 5 })).toBeNull()

    expect(drawings.local).toBe(5) // no waiting for the server
    expect(drawings.confirmed).toBe(0) // not the server's until it says so
    expect(engine.pending).toEqual([{ clientOpId: 'op-1', op: { add: 5 }, sent: true, applied: true }])
    expect(transport.sent).toEqual([{ clientOpId: 'op-1', baseSeq: 7, op: { add: 5 } }])
  })

  it('that the drawing refuses is neither kept nor sent', () => {
    const drawings = counters()
    const transport = wire()
    const engine = new SyncEngine(drawings, transport, 0, ids())

    expect(engine.local({ add: -1 })).toBe('would go below zero')

    expect(drawings.local).toBe(0)
    expect(engine.pending).toEqual([])
    expect(transport.sent).toEqual([])
  })

  it('gets its own client op ID', () => {
    const engine = new SyncEngine(counters(), wire(), 0, ids())
    engine.local({ add: 1 })
    engine.local({ add: 1 })
    expect(engine.pending.map((p) => p.clientOpId)).toEqual(['op-1', 'op-2'])
  })
})

describe('acks', () => {
  it('confirm the oldest pending op, advance the last seq, and leave what the user sees alone', () => {
    const drawings = counters()
    const transport = wire()
    const engine = new SyncEngine(drawings, transport, 10, ids())
    engine.local({ add: 1 })
    engine.local({ add: 2 })

    engine.ack('op-1', 11)

    expect(engine.lastSeq).toBe(11)
    expect(engine.pending.map((p) => p.clientOpId)).toEqual(['op-2'])
    expect([drawings.confirmed, drawings.local]).toEqual([1, 3])
    // Both were sent before either ack: from seq 10.
    expect(transport.sent.map((s) => s.baseSeq)).toEqual([10, 10])

    engine.ack('op-2', 14) // others' ops 12 and 13 came first
    expect(engine.lastSeq).toBe(14)
    expect(engine.synced).toBe(true)
    expect([drawings.confirmed, drawings.local]).toEqual([3, 3])
  })

  it('send later ops from the newer last seq', () => {
    const transport = wire()
    const engine = new SyncEngine(counters(), transport, 0, ids())
    engine.local({ add: 1 })
    engine.ack('op-1', 1)
    engine.local({ add: 1 })
    expect(transport.sent.map((s) => s.baseSeq)).toEqual([0, 1])
  })

  it("for an op that isn't pending, or a repeated one, change nothing", () => {
    const drawings = counters()
    const engine = new SyncEngine(drawings, wire(), 5, ids())
    engine.local({ add: 1 })
    engine.ack('op-1', 6)

    engine.ack('op-1', 6)
    engine.ack('nobody', 9)

    expect(engine.lastSeq).toBe(6)
    expect(drawings.confirmed).toBe(1) // applied once
  })
})

describe('without a connection', () => {
  it('still shows edits at once, and sends them in order once flushed', () => {
    const drawings = counters()
    const transport = wire()
    const engine = new SyncEngine(drawings, transport, 4, ids())
    transport.up = false

    engine.local({ add: 1 })
    engine.local({ add: 2 })
    expect(drawings.local).toBe(3)
    expect(engine.pending.map((p) => p.sent)).toEqual([false, false])
    expect(engine.synced).toBe(false)

    transport.up = true
    engine.flush()

    expect(transport.sent).toEqual([
      { clientOpId: 'op-1', baseSeq: 4, op: { add: 1 } },
      { clientOpId: 'op-2', baseSeq: 4, op: { add: 2 } },
    ])
    expect(engine.pending.map((p) => p.sent)).toEqual([true, true])
  })

  it('sends nothing twice when flushed again', () => {
    const transport = wire()
    const engine = new SyncEngine(counters(), transport, 0, ids())
    engine.local({ add: 1 })
    engine.flush()
    engine.flush()
    expect(transport.sent).toHaveLength(1)
  })

  it('keeps the order: an earlier unsent op goes before a later one', () => {
    const transport = wire()
    const engine = new SyncEngine(counters(), transport, 0, ids())
    transport.up = false
    engine.local({ add: 1 })
    transport.up = true
    engine.local({ add: 2 }) // the connection is back: op-1 goes first, then op-2

    expect(transport.sent.map((s) => s.clientOpId)).toEqual(['op-1', 'op-2'])
  })

  it('stops at the first op that cannot be sent, so no later one overtakes it', () => {
    const transport = wire()
    const engine = new SyncEngine(counters(), transport, 0, ids())
    transport.up = false
    engine.local({ add: 1 })
    engine.local({ add: 2 })
    engine.local({ add: 3 })
    // A connection that fails to send op-2 once, and would send anything else.
    let failedOnce = false
    transport.send = (clientOpId, baseSeq, op) => {
      if (clientOpId === 'op-2' && !failedOnce) return !(failedOnce = true)
      transport.sent.push({ clientOpId, baseSeq, op })
      return true
    }

    engine.flush()
    expect(transport.sent.map((s) => s.clientOpId)).toEqual(['op-1']) // op-3 waits behind op-2
    expect(engine.pending.map((p) => p.sent)).toEqual([true, false, false])

    engine.flush()
    expect(transport.sent.map((s) => s.clientOpId)).toEqual(['op-1', 'op-2', 'op-3'])
  })
})

describe("others' ops", () => {
  it('apply straight away when nothing is pending', () => {
    const drawings = counters(1)
    const engine = new SyncEngine(drawings, wire(), 3, ids())

    expect(engine.remote(4, { set: 10 })).toBe('applied')

    expect([drawings.confirmed, drawings.local]).toEqual([10, 10])
    expect(engine.lastSeq).toBe(4)
  })

  it("go underneath pending local ops, in the server's order", () => {
    const drawings = counters(1)
    const engine = new SyncEngine(drawings, wire(), 3, ids())
    engine.local({ add: 3 }) // 4 here

    // The server put this before ours: set to 10, then add 3.
    engine.remote(4, { set: 10 })

    expect(drawings.confirmed).toBe(10)
    expect(drawings.local).toBe(13) // not 10, as applying it on top would give
    expect(engine.pending.map((p) => p.applied)).toEqual([true])
  })

  it('reapply several pending ops in the order they were made', () => {
    const drawings = counters(0)
    const engine = new SyncEngine(drawings, wire(), 0, ids())
    engine.local({ set: 5 })
    engine.local({ add: 1 })

    engine.remote(1, { set: 100 })

    expect(drawings.local).toBe(6) // 100, then set 5, then add 1
  })

  it("are exact: the user sees precisely the server's order, with no floating-point drift", () => {
    const drawings = counters(0.3)
    const engine = new SyncEngine(drawings, wire(), 0, ids())
    engine.local({ add: 0.6 })

    engine.remote(1, { add: 0.1 })

    // The server's order: 0.3, then + 0.1, then + 0.6, which is exactly 1. Undoing + 0.6 and redoing it
    // would give 0.9999999999999999 instead (checked below, so this test can't pass by luck).
    expect(drawings.local).toBe(0.3 + 0.1 + 0.6)
    expect(0.3 + 0.6 - 0.6 + 0.1 + 0.6).not.toBe(0.3 + 0.1 + 0.6)
  })

  it('leave out a pending op that no longer applies, which the server will refuse too', () => {
    const drawings = counters(5)
    const engine = new SyncEngine(drawings, wire(), 0, ids())
    engine.local({ add: -5 }) // fine on 5

    engine.remote(1, { set: 2 }) // but not on 2

    expect(drawings.local).toBe(2)
    expect(engine.pending.map((p) => p.applied)).toEqual([false])
    expect(engine.diverged).toBe(false)
  })

  it('apply an op that no longer applied once a later remote op makes room again', () => {
    const drawings = counters(5)
    const engine = new SyncEngine(drawings, wire(), 0, ids())
    engine.local({ add: -5 })
    engine.remote(1, { set: 2 })
    expect(engine.pending[0].applied).toBe(false)

    engine.remote(2, { set: 7 })

    expect(drawings.local).toBe(2) // 7 - 5
    expect(engine.pending[0].applied).toBe(true)
  })

  it('that the client already has are ignored', () => {
    const drawings = counters(0)
    const engine = new SyncEngine(drawings, wire(), 5, ids())
    expect(engine.remote(5, { set: 99 })).toBe('duplicate')
    expect(engine.remote(2, { set: 99 })).toBe('duplicate')
    expect(drawings.local).toBe(0)
  })

  it('after a gap are left alone, so the missing ones can be fetched first', () => {
    const drawings = counters(0)
    const engine = new SyncEngine(drawings, wire(), 5, ids())
    expect(engine.remote(7, { set: 99 })).toBe('gap')
    expect(drawings.local).toBe(0)
    expect(engine.lastSeq).toBe(5)
  })

  it("mark the drawing as out of step if the server's op doesn't apply to the server's own state", () => {
    const engine = new SyncEngine(counters(0), wire(), 0, ids())
    expect(engine.remote(1, { add: -1 })).toBe('diverged')
    expect(engine.diverged).toBe(true)
  })
})

describe('an ack for an op the client left out', () => {
  it('marks the drawing as out of step: the server did apply it', () => {
    const engine = new SyncEngine(counters(5), wire(), 0, ids())
    engine.local({ add: -5 })
    engine.remote(1, { set: 2 })

    engine.ack('op-1', 2)

    expect(engine.diverged).toBe(true)
  })
})

describe('rejects', () => {
  it('roll back an op that was showing, keeping the pending ops after it', () => {
    const drawings = counters(0)
    const engine = new SyncEngine(drawings, wire(), 0, ids())
    engine.local({ set: 5 })
    engine.local({ add: 1 }) // 6

    const rejected = engine.reject('op-1')

    expect(rejected).toEqual({ set: 5 }) // the op, so the user can be told which edit it was
    expect(drawings.local).toBe(1) // 0 + 1: the set is gone, the add stays
    expect(engine.pending.map((p) => p.clientOpId)).toEqual(['op-2'])
  })

  it('leave the drawing alone for an op that was already left out', () => {
    const drawings = counters(5)
    const engine = new SyncEngine(drawings, wire(), 0, ids())
    engine.local({ add: -5 })
    engine.remote(1, { set: 2 }) // the add no longer applies
    let resets = 0
    const reset = drawings.resetLocal
    drawings.resetLocal = () => {
      resets++
      reset()
    }

    expect(engine.reject('op-1')).toEqual({ add: -5 })

    expect(drawings.local).toBe(2)
    expect(resets).toBe(0) // nothing to undo
    expect(engine.synced).toBe(true)
  })

  it("don't change what the server has, or the last seq", () => {
    const drawings = counters(3)
    const engine = new SyncEngine(drawings, wire(), 9, ids())
    engine.local({ add: 1 })
    engine.reject('op-1')
    expect([drawings.confirmed, drawings.local, engine.lastSeq]).toEqual([3, 3, 9])
  })

  it("for an op that isn't pending, or a repeated one, change nothing", () => {
    const drawings = counters(0)
    const engine = new SyncEngine(drawings, wire(), 0, ids())
    engine.local({ add: 1 })
    expect(engine.reject('nobody')).toBeNull()
    engine.reject('op-1')
    expect(engine.reject('op-1')).toBeNull()
    expect(drawings.local).toBe(0)
  })
})

describe('after reconnecting', () => {
  it('takes its own op in the catch-up as the ack it missed, so the op is applied once', () => {
    const drawings = counters(0)
    const engine = new SyncEngine(drawings, wire(), 0, ids())
    engine.local({ add: 2 }) // sent, but the connection dropped before the ack

    // The catch-up: someone else's op, then this client's own.
    engine.remote(1, { set: 10 }, 'theirs')
    expect(engine.remote(2, { add: 2 }, 'op-1')).toBe('applied')

    expect([drawings.confirmed, drawings.local]).toEqual([12, 12]) // not 14
    expect(engine.synced).toBe(true)
    expect(engine.lastSeq).toBe(2)
    engine.ack('op-1', 2) // the original ack, should it still turn up, changes nothing
    expect(drawings.confirmed).toBe(12)
  })

  it('sends every pending op again, with the same client op IDs, oldest first', () => {
    const transport = wire()
    const engine = new SyncEngine(counters(), transport, 0, ids())
    engine.local({ add: 1 })
    engine.local({ add: 2 })
    engine.local({ add: 3 })
    engine.ack('op-1', 1)

    engine.resend()

    expect(transport.sent.slice(3)).toEqual([
      { clientOpId: 'op-2', baseSeq: 1, op: { add: 2 } },
      { clientOpId: 'op-3', baseSeq: 1, op: { add: 3 } },
    ])
  })

  it("follows the server's order when an op is acked ahead of older pending ones whose rejects were lost", () => {
    const drawings = counters(0)
    const engine = new SyncEngine(drawings, wire(), 0, ids())
    engine.local({ set: 5 }) // refused, but the reject was lost with the connection
    engine.local({ add: 1 }) // 6

    engine.remote(1, { add: 1 }, 'op-2')

    // The server has 1; the set waits on top to be sent again.
    expect([drawings.confirmed, drawings.local]).toEqual([1, 5])
    expect(engine.pending.map((p) => p.clientOpId)).toEqual(['op-1'])
    expect(engine.diverged).toBe(false)
  })

  it('restarts from a snapshot, and a resent op the snapshot already holds is not applied twice', () => {
    const drawings = counters(0)
    const engine = new SyncEngine(drawings, wire(), 0, ids())
    engine.local({ add: 2 }) // stored as seq 5, but the ack was lost

    engine.loadSnapshot(7, { value: 2 }) // the server's drawing at 7, which includes it
    expect([drawings.confirmed, drawings.local, engine.lastSeq]).toEqual([2, 4, 7]) // shown twice, for now

    engine.resend()
    engine.ack('op-1', 5) // recognised: acked with its original seq

    expect([drawings.confirmed, drawings.local, engine.lastSeq]).toEqual([2, 2, 7])
    expect(engine.synced).toBe(true)
  })

  it("marks the drawing as out of step if the server's snapshot can't be loaded", () => {
    const engine = new SyncEngine(counters(0), wire(), 0, ids())
    engine.loadSnapshot(3, {})
    expect(engine.diverged).toBe(true)
    expect(engine.lastSeq).toBe(0)
  })
})
