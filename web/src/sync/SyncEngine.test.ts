import { describe, expect, it } from 'vitest'
import { SyncEngine, type DrawingModel, type Transport } from './SyncEngine'

// A drawing that's just a number: {add: n} adds n, {set: n} sets it. Refuses anything that would take it
// below 0, and records every op applied to it.
function counter(start = 0) {
  const applied: object[] = []
  const model: DrawingModel & { value: number; applied: object[] } = {
    value: start,
    applied,
    apply(op: object) {
      const { add, set } = op as { add?: number; set?: number }
      const next = set ?? model.value + (add ?? 0)
      if (next < 0) return 'would go below zero'
      model.value = next
      applied.push(op)
      return null
    },
    // A set's inverse depends on the value before it: the reason to invert before applying.
    invert(op: object) {
      const { add, set } = op as { add?: number; set?: number }
      return set === undefined ? { add: -(add ?? 0) } : { set: model.value }
    },
  }
  return model
}

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
  it('applies at once, waits for the server, and is sent from the last seq the client knows', () => {
    const model = counter()
    const transport = wire()
    const engine = new SyncEngine(model, transport, 7, ids())

    expect(engine.local({ add: 5 })).toBeNull()

    expect(model.value).toBe(5) // no waiting for the server
    expect(engine.pending).toEqual([{ clientOpId: 'op-1', op: { add: 5 }, inverse: { add: -5 }, sent: true }])
    expect(transport.sent).toEqual([{ clientOpId: 'op-1', baseSeq: 7, op: { add: 5 } }])
  })

  it('that the drawing refuses is neither kept nor sent', () => {
    const model = counter()
    const transport = wire()
    const engine = new SyncEngine(model, transport, 0, ids())

    expect(engine.local({ add: -1 })).toBe('would go below zero')

    expect(model.value).toBe(0)
    expect(engine.pending).toEqual([])
    expect(transport.sent).toEqual([])
  })

  it("keeps its inverse from before it was applied, for undoing it later", () => {
    const engine = new SyncEngine(counter(4), wire(), 0, ids())
    engine.local({ set: 9 })
    expect(engine.pending[0].inverse).toEqual({ set: 4 }) // back to the value before, not the one after
  })

  it('gets its own client op ID', () => {
    const engine = new SyncEngine(counter(), wire(), 0, ids())
    engine.local({ add: 1 })
    engine.local({ add: 1 })
    expect(engine.pending.map((p) => p.clientOpId)).toEqual(['op-1', 'op-2'])
  })
})

describe('acks', () => {
  it('confirm the oldest pending op and advance the last seq', () => {
    const transport = wire()
    const engine = new SyncEngine(counter(), transport, 10, ids())
    engine.local({ add: 1 })
    engine.local({ add: 2 })

    engine.ack('op-1', 11)

    expect(engine.lastSeq).toBe(11)
    expect(engine.pending.map((p) => p.clientOpId)).toEqual(['op-2'])
    // Both were sent before either ack: from seq 10.
    expect(transport.sent.map((s) => s.baseSeq)).toEqual([10, 10])

    engine.ack('op-2', 14) // others' ops 12 and 13 came first
    expect(engine.lastSeq).toBe(14)
    expect(engine.pending).toEqual([])
    expect(engine.synced).toBe(true)
  })

  it('send later ops from the newer last seq', () => {
    const transport = wire()
    const engine = new SyncEngine(counter(), transport, 0, ids())
    engine.local({ add: 1 })
    engine.ack('op-1', 1)
    engine.local({ add: 1 })
    expect(transport.sent.map((s) => s.baseSeq)).toEqual([0, 1])
  })

  it('for an op that isn\'t pending, or a repeated one, change nothing', () => {
    const engine = new SyncEngine(counter(), wire(), 5, ids())
    engine.local({ add: 1 })
    engine.ack('op-1', 6)

    engine.ack('op-1', 6)
    engine.ack('nobody', 9)

    expect(engine.lastSeq).toBe(6)
    expect(engine.pending).toEqual([])
  })

  it('never move the last seq backwards', () => {
    const engine = new SyncEngine(counter(), wire(), 20, ids())
    engine.local({ add: 1 })
    engine.ack('op-1', 3)
    expect(engine.lastSeq).toBe(20)
  })
})

describe('without a connection', () => {
  it('still applies edits at once, and sends them in order once flushed', () => {
    const model = counter()
    const transport = wire()
    const engine = new SyncEngine(model, transport, 4, ids())
    transport.up = false

    engine.local({ add: 1 })
    engine.local({ add: 2 })
    expect(model.value).toBe(3)
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
    const engine = new SyncEngine(counter(), transport, 0, ids())
    engine.local({ add: 1 })
    engine.flush()
    engine.flush()
    expect(transport.sent).toHaveLength(1)
  })

  it('keeps the order: an earlier unsent op goes before a later one', () => {
    const transport = wire()
    const engine = new SyncEngine(counter(), transport, 0, ids())
    transport.up = false
    engine.local({ add: 1 })
    transport.up = true
    engine.local({ add: 2 }) // the connection is back: op-1 goes first, then op-2

    expect(transport.sent.map((s) => s.clientOpId)).toEqual(['op-1', 'op-2'])
  })

  it('stops at the first op that cannot be sent, so no later one overtakes it', () => {
    const transport = wire()
    const engine = new SyncEngine(counter(), transport, 0, ids())
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
