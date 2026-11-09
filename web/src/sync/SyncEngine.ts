// The client side of keeping a drawing in step with the server. Framework-free, and independent of both
// the WebSocket (a Transport) and the WebAssembly core (a DrawingModel), so its rules can be tested alone.
//
// A local edit is applied at once, so the editor never waits for the network, and kept as pending until the
// server acks it with its seq. Pending ops are sent in the order they were made; while the connection is
// down they wait, and flush() sends them once it's back.

// The drawing the engine edits.
export interface DrawingModel {
  // Applies an op: null, or why it doesn't apply (and nothing changed).
  apply(op: object): string | null
  // The op that undoes `op`, which hasn't been applied yet; null if there isn't one.
  invert(op: object): object | null
}

// How ops reach the server.
export interface Transport {
  // False if it couldn't be sent now, such as while disconnected.
  send(clientOpId: string, baseSeq: number, op: object): boolean
}

export interface PendingOp {
  clientOpId: string // the server's way of recognising a resend, so it's never applied twice
  op: object
  inverse: object // taken before the op was applied: undoes it, for a rebase or a reject
  sent: boolean
}

export class SyncEngine {
  // The seq of the newest op the server has confirmed to this client: its own acked ops and others'.
  lastSeq: number
  // Local ops the server hasn't acked yet, oldest first, all already applied to the drawing.
  pending: PendingOp[] = []

  private readonly model: DrawingModel
  private readonly transport: Transport
  private readonly newClientOpId: () => string

  constructor(
    model: DrawingModel,
    transport: Transport,
    lastSeq: number,
    newClientOpId: () => string = () => crypto.randomUUID(),
  ) {
    this.model = model
    this.transport = transport
    this.lastSeq = lastSeq
    this.newClientOpId = newClientOpId
  }

  // Whether the server has every local edit.
  get synced(): boolean {
    return this.pending.length === 0
  }

  // Makes a local edit: applies it now and sends it to the server. Null, or why the drawing refused it, in
  // which case nothing is kept or sent.
  local(op: object): string | null {
    const inverse = this.model.invert(op)
    if (!inverse) return "that edit can't be applied to the drawing"
    const refused = this.model.apply(op)
    if (refused) return refused
    this.pending.push({ clientOpId: this.newClientOpId(), op, inverse, sent: false })
    this.flush()
    return null
  }

  // The server stored a local op as `seq`. Acks for ops that aren't pending, such as a repeat, change nothing.
  ack(clientOpId: string, seq: number) {
    const index = this.pending.findIndex((pending) => pending.clientOpId === clientOpId)
    if (index === -1) return
    this.pending.splice(index, 1)
    this.lastSeq = Math.max(this.lastSeq, seq)
  }

  // Sends the pending ops not sent yet, oldest first, stopping at the first that can't go: a later op must
  // never reach the server before an earlier one.
  flush() {
    for (const pending of this.pending) {
      if (pending.sent) continue
      if (!this.transport.send(pending.clientOpId, this.lastSeq, pending.op)) return
      pending.sent = true
    }
  }
}
