import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest'
import { DocumentSync, opBurst, opsPerSecond, reconnectDelayMs, type DocumentSyncEvents } from './DocumentSync'
import { counters, FakeSocket } from './testing'

function setUp({ renews = true } = {}) {
  const sockets: FakeSocket[] = []
  const drawings = counters(0)
  let token = 'tok'
  let n = 0
  const events = {
    onStatus: vi.fn(), onChange: vi.fn(), onRejected: vi.fn(), onPresence: vi.fn(), onDiverged: vi.fn(), onStopped: vi.fn(),
  } satisfies DocumentSyncEvents
  const renewToken = vi.fn(async () => {
    if (renews) token = `renewed-${renewToken.mock.calls.length}`
    return renews
  })
  const sync = new DocumentSync({
    documentId: 'd1',
    drawings,
    lastSeq: 0,
    getToken: () => token,
    renewToken,
    events,
    origin: 'https://draw.example',
    openSocket: (url) => {
      const socket = new FakeSocket(url)
      sockets.push(socket)
      return socket
    },
    delayMs: (attempt) => [100, 200, 400][attempt] ?? 800,
    newClientOpId: () => `op-${++n}`,
  })
  const latest = () => sockets[sockets.length - 1]
  // Opens the newest socket and plays the server up to `joined`.
  const join = (joined: object = { headSeq: 0, ops: [] }) => {
    latest().opened()
    latest().receive({ type: 'ready', role: 'editor' })
    latest().receive({ type: 'joined', ...joined })
  }
  const opsSent = (socket: FakeSocket) =>
    socket.sent.filter((m) => (m as { type: string }).type === 'op').map((m) => (m as { clientOpId: string }).clientOpId)
  return { sync, sockets, drawings, events, renewToken, latest, join, opsSent }
}

// Lets the token renewal, a promise, settle.
const settle = () => vi.advanceTimersByTimeAsync(0)

beforeEach(() => vi.useFakeTimers())
afterEach(() => vi.useRealTimers())

describe('reconnectDelayMs', () => {
  it('doubles from 1 s up to 30 s, at a random point in the upper half', () => {
    expect([0, 1, 2, 5, 20].map((attempt) => reconnectDelayMs(attempt, () => 0))).toEqual([500, 1000, 2000, 15_000, 15_000])
    expect([0, 1, 2, 5, 20].map((attempt) => reconnectDelayMs(attempt, () => 1))).toEqual([1000, 2000, 4000, 30_000, 30_000])
  })
})

describe('DocumentSync', () => {
  it('reconnects after a drop, waiting longer each time, and catches up from the last seq it has', async () => {
    const { sync, sockets, latest, join } = setUp()
    sync.start()
    join()
    latest().receive({ type: 'op', seq: 1, userId: 'bob', clientOpId: 'b1', op: { add: 1 } })

    latest().dropped(1006)
    await vi.advanceTimersByTimeAsync(99)
    expect(sockets).toHaveLength(1)
    await vi.advanceTimersByTimeAsync(1)
    expect(sockets).toHaveLength(2)

    latest().dropped(1006) // before joining: the next wait is longer
    await vi.advanceTimersByTimeAsync(199)
    expect(sockets).toHaveLength(2)
    await vi.advanceTimersByTimeAsync(1)
    expect(sockets).toHaveLength(3)

    latest().opened()
    latest().receive({ type: 'ready', role: 'editor' })
    expect(latest().sent).toEqual([{ type: 'join', lastSeq: 1 }])
    latest().receive({ type: 'joined', headSeq: 1, ops: [] })

    latest().dropped(1006) // joined in between: back to the shortest wait
    await vi.advanceTimersByTimeAsync(100)
    expect(sockets).toHaveLength(4)
  })

  it("resends pending ops with the same client op IDs, after taking its own op in the catch-up as its ack", async () => {
    const { sync, sockets, drawings, latest, join, opsSent } = setUp()
    sync.start()
    join()
    sync.edit({ add: 1 })
    sync.edit({ add: 2 })
    expect(opsSent(sockets[0])).toEqual(['op-1', 'op-2'])

    latest().dropped(1006) // before either ack
    await vi.advanceTimersByTimeAsync(100)
    join({ headSeq: 1, ops: [{ seq: 1, userId: 'me', clientOpId: 'op-1', op: { add: 1 } }] })

    expect(opsSent(sockets[1])).toEqual(['op-2']) // op-1 is stored: not sent again
    expect([drawings.confirmed, drawings.local]).toEqual([1, 3])
  })

  it('sends edits made while offline once it has rejoined, in order', async () => {
    const { sync, sockets, latest, join, opsSent } = setUp()
    sync.start()
    join()
    latest().dropped(1006)

    sync.edit({ add: 1 })
    sync.edit({ add: 2 })
    await vi.advanceTimersByTimeAsync(100)
    latest().opened()
    latest().receive({ type: 'ready', role: 'editor' })
    expect(opsSent(sockets[1])).toEqual([]) // not before joining

    latest().receive({ type: 'joined', headSeq: 0, ops: [] })
    expect(opsSent(sockets[1])).toEqual(['op-1', 'op-2'])
  })

  it("gives up when the document is gone or the user isn't a member", async () => {
    const { sync, sockets, events, latest } = setUp()
    sync.start()
    latest().dropped(4404)
    await vi.advanceTimersByTimeAsync(10_000)
    expect(sockets).toHaveLength(1)
    expect(events.onStopped).toHaveBeenCalledExactlyOnceWith('not_found')
  })

  it('reconnects with a new token when the old one has expired', async () => {
    const { sync, sockets, renewToken, latest } = setUp()
    sync.start()
    latest().dropped(4401)
    await settle()
    expect(renewToken).toHaveBeenCalledOnce()
    await vi.advanceTimersByTimeAsync(100)
    expect(sockets[1].url).toContain('token=renewed-1')
  })

  it("gives up when the token can't be renewed: the login has ended", async () => {
    const { sync, sockets, events, latest } = setUp({ renews: false })
    sync.start()
    latest().dropped(4401)
    await vi.advanceTimersByTimeAsync(10_000)
    expect(sockets).toHaveLength(1)
    expect(events.onStopped).toHaveBeenCalledExactlyOnceWith('signed_out')
  })

  it('reconnects at once when asked, instead of waiting out the delay', async () => {
    const { sync, sockets, latest, join } = setUp()
    sync.start()
    join()
    sync.reconnectNow() // connected: nothing to do
    expect(sockets).toHaveLength(1)

    latest().dropped(1006)
    sync.reconnectNow()
    expect(sockets).toHaveLength(2)
    await vi.advanceTimersByTimeAsync(10_000)
    expect(sockets).toHaveLength(2) // the waiting reconnect was cancelled, not made as well
  })

  it("never sends ops faster than the server takes them: a burst waits its turn, in order", async () => {
    const { sync, latest, join, opsSent } = setUp()
    sync.start()
    join()
    for (let i = 0; i < opBurst + 20; i++) sync.edit({ add: 1 })
    expect(opsSent(latest())).toHaveLength(opBurst) // a burst at once, no more

    await vi.advanceTimersByTimeAsync(400) // 10 more ops' worth of time
    expect(opsSent(latest())).toHaveLength(opBurst + 0.4 * opsPerSecond)
    await vi.advanceTimersByTimeAsync(1000)
    const sent = opsSent(latest())
    expect(sent).toHaveLength(opBurst + 20)
    expect(sent).toEqual(Array.from({ length: opBurst + 20 }, (_, i) => `op-${i + 1}`))
  })

  it('stops retrying once stopped', async () => {
    const { sync, sockets, latest } = setUp()
    sync.start()
    latest().dropped(1006)
    sync.stop()
    await vi.advanceTimersByTimeAsync(10_000)
    expect(sockets).toHaveLength(1)
  })

  it('rejoins to catch up when ops go missing on a live connection', () => {
    const { sync, sockets, drawings, latest, join } = setUp()
    sync.start()
    join()
    latest().receive({ type: 'op', seq: 2, userId: 'bob', clientOpId: 'b2', op: { add: 1 } }) // 1 is missing

    expect(sockets).toHaveLength(2)
    expect(sockets[0].closedWith).toBe(1000)
    expect(drawings.local).toBe(0)
    latest().opened()
    latest().receive({ type: 'ready', role: 'editor' })
    expect(latest().sent).toEqual([{ type: 'join', lastSeq: 0 }])
  })

  it("sends this user's pointer once live, and reports who else is there", () => {
    const { sync, events, latest, join } = setUp()
    sync.start()
    sync.cursor(1, 2) // not connected yet: dropped
    join()
    sync.cursor(3, 4)
    expect(latest().sent.filter((m) => (m as { type: string }).type === 'cursor')).toEqual([{ type: 'cursor', x: 3, y: 4 }])

    const users = [{ id: 'u2', name: 'bob', cursor: { x: 5, y: 6 } }]
    latest().receive({ type: 'presence', users })
    expect(events.onPresence).toHaveBeenCalledExactlyOnceWith(users)
  })

  it('reports rejected edits with the op, and changes to what the user sees', () => {
    const { sync, events, latest, join } = setUp()
    sync.start()
    join()
    sync.edit({ add: 1 })
    latest().receive({ type: 'reject', clientOpId: 'op-1', reason: 'rate_limited', message: 'slow down' })
    expect(events.onRejected).toHaveBeenCalledExactlyOnceWith({ add: 1 }, 'rate_limited', 'slow down')
    expect(events.onChange).toHaveBeenCalled()
  })

  it('stops, and says so, when the drawing no longer matches the server', () => {
    const { sync, events, latest, join } = setUp()
    sync.start()
    join()
    latest().receive({ type: 'op', seq: 1, userId: 'bob', clientOpId: 'b1', op: { add: -1 } }) // can't apply
    expect(events.onDiverged).toHaveBeenCalledOnce()
    expect(latest().closedWith).toBe(1000)
  })
})
