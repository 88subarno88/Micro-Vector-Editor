import {
  parseServerMessage,
  type ClientMessage,
  type CommittedOp,
  type PresenceUser,
  type Role,
  type ServerMessage,
} from './messages'

// One connection to a document's WebSocket. Framework-free: it knows the protocol, not the editor. It
// opens the socket with an access token, joins from `lastSeq` once the server is ready, keeps the
// connection alive with pings, and reports what arrives.

// The server closes a connection silent for 45 s; this leaves plenty of room.
export const pingIntervalMs = 15_000
// Reported as the close code when no access token could be had, so the socket was never opened: the same
// as the server's for a bad token.
export const closeWithoutToken = 4401

export type SyncStatus =
  | 'connecting' // getting a token and opening the socket
  | 'joining' // the server has checked the token; waiting for the drawing to load
  | 'live' // joined: ops can be sent, and others' arrive
  | 'closed'

// What the editor hears about. Every handler is optional.
export interface SyncEvents {
  onStatus?(status: SyncStatus): void
  onReady?(role: Role): void
  onJoined?(joined: { headSeq: number; ops: CommittedOp[] } | { headSeq: number; snapshot: object }): void
  onOp?(op: CommittedOp): void
  onAck?(clientOpId: string, seq: number): void
  onReject?(clientOpId: string, reason: string, message: string): void
  onPresence?(users: PresenceUser[]): void
  onError?(code: string, message: string): void
  onClosed?(code: number, reason: string): void
}

// The part of a WebSocket this uses, so tests can stand in for one.
export interface SocketLike {
  readyState: number
  send(data: string): void
  close(code?: number, reason?: string): void
  onopen: ((event: Event) => void) | null
  onmessage: ((event: MessageEvent) => void) | null
  onclose: ((event: CloseEvent) => void) | null
}

export interface SyncOptions {
  documentId: string
  // The current access token; null if there isn't one yet.
  getToken(): string | null
  // Gets a new one with the refresh cookie. True if it worked.
  renewToken(): Promise<boolean>
  events: SyncEvents
  // Where to connect; the page's own host by default.
  origin?: string
  openSocket?(url: string): SocketLike
}

const open = 1

// The socket URL for a document, on the page's own host: wss for https, ws for http.
export function socketUrl(origin: string, documentId: string, token: string): string {
  const url = new URL(`/ws/docs/${encodeURIComponent(documentId)}`, origin)
  url.protocol = url.protocol === 'https:' ? 'wss:' : 'ws:'
  url.searchParams.set('token', token)
  return url.toString()
}

export class SyncClient {
  status: SyncStatus = 'closed'
  private socket: SocketLike | null = null
  private pinger: ReturnType<typeof setInterval> | null = null
  private readonly options: SyncOptions

  constructor(options: SyncOptions) {
    this.options = options
  }

  // Connects and joins, asking for every op after `lastSeq` (0 for the whole drawing).
  async connect(lastSeq: number): Promise<void> {
    this.close()
    this.setStatus('connecting')
    const { getToken, renewToken, documentId } = this.options
    // Without a token, as after a reload, get one from the refresh cookie first.
    if (!getToken() && !(await renewToken())) return this.closed(closeWithoutToken, 'no access token')
    if (this.status !== 'connecting') return // closed while waiting

    const origin = this.options.origin ?? globalThis.location.origin
    const socket = (this.options.openSocket ?? ((url) => new WebSocket(url)))(
      socketUrl(origin, documentId, getToken()!),
    )
    this.socket = socket
    socket.onopen = () => {
      this.pinger = setInterval(() => this.send({ type: 'ping' }), pingIntervalMs)
    }
    socket.onmessage = (event) => {
      if (this.socket !== socket) return
      const message = parseServerMessage(String(event.data))
      if (message) this.handle(message, lastSeq)
    }
    socket.onclose = (event) => {
      if (this.socket === socket) this.closed(event.code, event.reason)
    }
  }

  // Sends an op; false, sending nothing, unless the connection is live.
  sendOp(clientOpId: string, baseSeq: number, op: object): boolean {
    if (this.status !== 'live') return false
    return this.send({ type: 'op', clientOpId, baseSeq, op })
  }

  sendCursor(x: number, y: number): boolean {
    if (this.status !== 'live') return false
    return this.send({ type: 'cursor', x, y })
  }

  // Closes the connection, if there is one. Nothing more is reported for it.
  close() {
    const socket = this.socket
    this.socket = null
    this.stopPinging()
    socket?.close(1000, 'closed by the client')
    if (this.status !== 'closed') this.setStatus('closed')
  }

  private handle(message: ServerMessage, lastSeq: number) {
    const { events } = this.options
    switch (message.type) {
      case 'ready':
        this.setStatus('joining')
        events.onReady?.(message.role)
        this.send({ type: 'join', lastSeq })
        break
      case 'joined': {
        const { type: _, ...joined } = message
        this.setStatus('live')
        events.onJoined?.(joined)
        break
      }
      case 'op': {
        const { type: _, ...op } = message
        events.onOp?.(op)
        break
      }
      case 'ack':
        events.onAck?.(message.clientOpId, message.seq)
        break
      case 'reject':
        events.onReject?.(message.clientOpId, message.reason, message.message)
        break
      case 'presence':
        events.onPresence?.(message.users)
        break
      case 'error':
        events.onError?.(message.code, message.message)
        break
      case 'pong':
        break
    }
  }

  private send(message: ClientMessage): boolean {
    if (this.socket?.readyState !== open) return false
    this.socket.send(JSON.stringify(message))
    return true
  }

  private closed(code: number, reason: string) {
    this.socket = null
    this.stopPinging()
    this.setStatus('closed')
    this.options.events.onClosed?.(code, reason)
  }

  private stopPinging() {
    if (this.pinger !== null) clearInterval(this.pinger)
    this.pinger = null
  }

  private setStatus(status: SyncStatus) {
    this.status = status
    this.options.events.onStatus?.(status)
  }
}
