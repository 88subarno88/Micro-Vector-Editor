import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest'
import { closeWithoutToken, pingIntervalMs, socketUrl, SyncClient, type SyncEvents } from './SyncClient'
import { FakeSocket } from './testing'

function setUp({ token = 'tok' as string | null, renews = true } = {}) {
  const sockets: FakeSocket[] = []
  let current = token
  const events = {
    onStatus: vi.fn(), onReady: vi.fn(), onJoined: vi.fn(), onOp: vi.fn(), onAck: vi.fn(),
    onReject: vi.fn(), onPresence: vi.fn(), onError: vi.fn(), onClosed: vi.fn(),
  } satisfies SyncEvents
  const renewToken = vi.fn(async () => {
    if (renews) current = 'renewed'
    return renews
  })
  const client = new SyncClient({
    documentId: 'doc 1',
    origin: 'https://draw.example',
    getToken: () => current,
    renewToken,
    events,
    openSocket: (url) => {
      const socket = new FakeSocket(url)
      sockets.push(socket)
      return socket
    },
  })
  return { client, sockets, events, renewToken }
}

// Connects, opens the socket and plays the server up to `joined`.
async function joined(lastSeq = 0) {
  const setup = setUp()
  await setup.client.connect(lastSeq)
  const socket = setup.sockets[0]
  socket.opened()
  socket.receive({ type: 'ready', role: 'editor' })
  socket.receive({ type: 'joined', headSeq: 5, ops: [] })
  return { ...setup, socket }
}

beforeEach(() => vi.useFakeTimers())
afterEach(() => vi.useRealTimers())

describe('socketUrl', () => {
  it('uses wss on an https page, ws on an http one, and encodes the token', () => {
    expect(socketUrl('https://draw.example', 'd1', 'a+b/c')).toBe('wss://draw.example/ws/docs/d1?token=a%2Bb%2Fc')
    expect(socketUrl('http://localhost:5173', 'd1', 't')).toBe('ws://localhost:5173/ws/docs/d1?token=t')
  })
})

describe('SyncClient', () => {
  it('joins from lastSeq once the server is ready, then is live', async () => {
    const { client, sockets, events } = setUp()
    await client.connect(42)
    const socket = sockets[0]
    expect(socket.url).toBe('wss://draw.example/ws/docs/doc%201?token=tok')
    socket.opened()
    expect(socket.sent).toEqual([]) // nothing before ready: the server would refuse it

    socket.receive({ type: 'ready', role: 'editor' })
    expect(socket.sent).toEqual([{ type: 'join', lastSeq: 42 }])
    expect(client.status).toBe('joining')
    expect(events.onReady).toHaveBeenCalledWith('editor')

    socket.receive({ type: 'joined', headSeq: 43, ops: [{ seq: 43, userId: 'u', clientOpId: 'c', op: { kind: 'deleteShape', id: 'a' } }] })
    expect(client.status).toBe('live')
    expect(events.onJoined).toHaveBeenCalledWith({ headSeq: 43, ops: [{ seq: 43, userId: 'u', clientOpId: 'c', op: { kind: 'deleteShape', id: 'a' } }] })
    expect(events.onStatus.mock.calls.map(([status]) => status)).toEqual(['connecting', 'joining', 'live'])
  })

  it('passes on what arrives, and ignores what it cannot read', async () => {
    const { socket, events } = await joined()
    socket.receive({ type: 'op', seq: 6, userId: 'u', clientOpId: 'c', op: { kind: 'deleteShape', id: 'a' } })
    socket.receive({ type: 'ack', clientOpId: 'c1', seq: 7 })
    socket.receive({ type: 'reject', clientOpId: 'c2', reason: 'forbidden', message: 'viewers cannot edit' })
    socket.receive({ type: 'presence', users: [] })
    socket.receive({ type: 'error', code: 'busy', message: 'try again' })
    socket.receive({ type: 'op', seq: 'eight' })
    socket.onmessage?.({ data: '{oops' } as MessageEvent)

    expect(events.onOp).toHaveBeenCalledExactlyOnceWith({ seq: 6, userId: 'u', clientOpId: 'c', op: { kind: 'deleteShape', id: 'a' } })
    expect(events.onAck).toHaveBeenCalledWith('c1', 7)
    expect(events.onReject).toHaveBeenCalledWith('c2', 'forbidden', 'viewers cannot edit')
    expect(events.onPresence).toHaveBeenCalledWith([])
    expect(events.onError).toHaveBeenCalledWith('busy', 'try again')
  })

  it('sends ops and cursors only while live', async () => {
    const { client, sockets } = setUp()
    await client.connect(0)
    sockets[0].opened()
    expect(client.sendOp('c1', 0, { kind: 'deleteShape', id: 'a' })).toBe(false)

    sockets[0].receive({ type: 'ready', role: 'owner' })
    sockets[0].receive({ type: 'joined', headSeq: 0, ops: [] })
    expect(client.sendOp('c1', 0, { kind: 'deleteShape', id: 'a' })).toBe(true)
    expect(client.sendCursor(3, 4)).toBe(true)
    expect(sockets[0].sent.slice(1)).toEqual([
      { type: 'op', clientOpId: 'c1', baseSeq: 0, op: { kind: 'deleteShape', id: 'a' } },
      { type: 'cursor', x: 3, y: 4 },
    ])
  })

  it('pings to stay connected, and stops when closed', async () => {
    const { client, socket } = await joined()
    const pings = () => socket.sent.filter((m) => (m as { type: string }).type === 'ping').length
    vi.advanceTimersByTime(pingIntervalMs * 3)
    expect(pings()).toBe(3)
    client.close()
    vi.advanceTimersByTime(pingIntervalMs * 3)
    expect(pings()).toBe(3)
  })

  it('gets a token with the refresh cookie when it has none', async () => {
    const { client, sockets, renewToken } = setUp({ token: null })
    await client.connect(0)
    expect(renewToken).toHaveBeenCalledOnce()
    expect(sockets[0].url).toContain('token=renewed')
  })

  it('reports a close without opening a socket when no token can be had', async () => {
    const { client, sockets, events } = setUp({ token: null, renews: false })
    await client.connect(0)
    expect(sockets).toEqual([])
    expect(client.status).toBe('closed')
    expect(events.onClosed).toHaveBeenCalledWith(closeWithoutToken, 'no access token')
  })

  it("reports the server's close code", async () => {
    const { client, socket, events } = await joined()
    socket.dropped(4404, 'not_found')
    expect(client.status).toBe('closed')
    expect(events.onClosed).toHaveBeenCalledWith(4404, 'not_found')
  })

  it('reports nothing for a connection it closed itself', async () => {
    const { client, socket, events } = await joined()
    client.close()
    socket.dropped(1000)
    socket.receive({ type: 'op', seq: 6, userId: 'u', clientOpId: 'c', op: {} })
    expect(socket.closedWith).toBe(1000)
    expect(events.onClosed).not.toHaveBeenCalled()
    expect(events.onOp).not.toHaveBeenCalled()
  })
})
