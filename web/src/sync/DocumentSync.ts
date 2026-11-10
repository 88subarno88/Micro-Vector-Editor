import { closeCodes, type CommittedOp, type PresenceUser } from './messages'
import { SyncClient, type SocketLike, type SyncStatus } from './SyncClient'
import { SyncEngine, type Drawings } from './SyncEngine'

// A document kept in sync for as long as it's open: the engine over a connection that comes back by itself.
// When the connection drops, it reconnects after a growing delay, catches up from the last seq it has, and
// resends whatever is pending (see SyncEngine). It gives up only where retrying can't help: the document is
// gone or the user is no longer a member, or the login has ended.

// The wait before reconnect attempt `attempt` (0 first): doubling from 1 s up to 30 s, at a random point
// in its upper half, so clients dropped together don't all come back at the same moment.
export function reconnectDelayMs(attempt: number, random = Math.random): number {
  const ceiling = Math.min(30_000, 1000 * 2 ** attempt)
  return ceiling / 2 + random() * (ceiling / 2)
}

export type StopReason =
  | 'not_found' // the document is gone, or the user is no longer a member
  | 'signed_out' // no access token could be had

export interface DocumentSyncEvents {
  onStatus?(status: SyncStatus): void
  // What the user sees may have changed: others' ops arrived, or an edit was rolled back.
  onChange?(): void
  onRejected?(op: object, reason: string, message: string): void
  // Everyone looking at the document, this user included, whenever someone joins, leaves or moves.
  onPresence?(users: PresenceUser[]): void
  // The drawing no longer matches the server's (see SyncEngine.diverged): syncing has stopped, and the
  // document should be opened afresh.
  onDiverged?(): void
  onStopped?(reason: StopReason): void
}

export interface DocumentSyncOptions {
  documentId: string
  drawings: Drawings
  lastSeq: number // the seq the drawings are at
  getToken(): string | null
  renewToken(): Promise<boolean>
  events: DocumentSyncEvents
  origin?: string
  openSocket?(url: string): SocketLike
  delayMs?(attempt: number): number
  newClientOpId?(): string
}

export class DocumentSync {
  readonly engine: SyncEngine
  private readonly client: SyncClient
  private readonly options: DocumentSyncOptions
  private attempt = 0 // reconnects since the last successful join
  private retry: ReturnType<typeof setTimeout> | null = null
  private stopped = false

  constructor(options: DocumentSyncOptions) {
    this.options = options
    const { documentId, getToken, renewToken, origin, openSocket, events } = options
    this.client = new SyncClient({
      documentId,
      getToken,
      renewToken,
      origin,
      openSocket,
      events: {
        onStatus: (status) => events.onStatus?.(status),
        onJoined: (joined) => this.joined(joined),
        onOp: (committed) => this.received(committed),
        onAck: (clientOpId, seq) => {
          this.engine.ack(clientOpId, seq)
          this.changed()
        },
        onReject: (clientOpId, reason, message) => {
          const op = this.engine.reject(clientOpId)
          if (!op) return
          events.onRejected?.(op, reason, message)
          this.changed()
        },
        onPresence: (users) => events.onPresence?.(users),
        onClosed: (code) => void this.closed(code),
      },
    })
    this.engine = new SyncEngine(
      options.drawings,
      { send: (clientOpId, baseSeq, op) => this.client.sendOp(clientOpId, baseSeq, op) },
      options.lastSeq,
      options.newClientOpId,
    )
  }

  start() {
    void this.client.connect(this.engine.lastSeq)
  }

  // Makes a local edit: null, or why the drawing refused it (see SyncEngine.local).
  edit(op: object): string | null {
    return this.engine.local(op)
  }

  // Tells the others where this user's pointer is, in drawing units. Dropped unless connected: only the
  // latest position matters.
  cursor(x: number, y: number) {
    this.client.sendCursor(x, y)
  }

  stop() {
    this.stopped = true
    if (this.retry !== null) clearTimeout(this.retry)
    this.retry = null
    this.client.close()
  }

  private joined(joined: { headSeq: number; ops: CommittedOp[] } | { headSeq: number; snapshot: object }) {
    this.attempt = 0
    if ('snapshot' in joined) this.engine.loadSnapshot(joined.headSeq, joined.snapshot)
    else for (const { seq, op, clientOpId } of joined.ops) this.engine.remote(seq, op, clientOpId)
    this.engine.resend()
    this.changed()
  }

  private received({ seq, op, clientOpId }: CommittedOp) {
    // Ops arrive in order on one connection, so a gap means some went missing: rejoin to catch up.
    if (this.engine.remote(seq, op, clientOpId) === 'gap') void this.client.connect(this.engine.lastSeq)
    else this.changed()
  }

  private async closed(code: number) {
    if (this.stopped) return
    if (code === closeCodes.notFound) return this.stopWith('not_found')
    // The token expired, or there was none: reconnecting needs a new one.
    if (code === closeCodes.unauthorized && !(await this.options.renewToken())) return this.stopWith('signed_out')
    if (this.stopped) return // stopped while renewing
    const delay = (this.options.delayMs ?? reconnectDelayMs)(this.attempt++)
    this.retry = setTimeout(() => {
      this.retry = null
      void this.client.connect(this.engine.lastSeq)
    }, delay)
  }

  private changed() {
    if (!this.engine.diverged) return this.options.events.onChange?.()
    this.stop()
    this.options.events.onDiverged?.()
  }

  private stopWith(reason: StopReason) {
    this.stop()
    this.options.events.onStopped?.(reason)
  }
}
