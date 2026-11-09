// The client side of keeping a drawing in step with the server. Framework-free, and independent of both
// the WebSocket (a Transport) and the WebAssembly core (Drawings), so its rules can be tested alone.
//
// A local edit is applied at once, so the editor never waits for the network, and kept as pending until the
// server acks it with its seq. Pending ops are sent in the order they were made; while the connection is
// down they wait, and flush() sends them once it's back.
//
// Two drawings are kept. The confirmed one is the server's state as far as this client knows it: only ops the
// server committed, in its order. The local one, which the user sees, is the confirmed one with the pending
// ops on top. Others' ops come with seqs lower than any pending op's, since the server ordered them first;
// each is applied to the confirmed drawing, and the local one is rebuilt from it with the pending ops
// reapplied. Rebuilding rather than undoing keeps it exact: undoing a resize or move with its inverse would
// leave floating-point drift. A pending op that no longer applies is left out; the server, running the same
// core on the same state in the same order, will refuse it too; a reject then drops it. Refused ops are
// dropped whatever the reason, even "too many edits" or a server error: the server has moved on without them,
// and resending one later would put it out of order.

export interface Drawings {
  // Each applies an op: null, or why it doesn't apply (and nothing changed).
  applyConfirmed(op: object): string | null
  applyLocal(op: object): string | null
  // Makes the local drawing a copy of the confirmed one.
  resetLocal(): void
}

// How ops reach the server.
export interface Transport {
  // False if it couldn't be sent now, such as while disconnected.
  send(clientOpId: string, baseSeq: number, op: object): boolean
}

export interface PendingOp {
  clientOpId: string // the server's way of recognising a resend, so it's never applied twice
  op: object
  sent: boolean
  applied: boolean // false while it doesn't apply on top of others' ops; the server will refuse it then
}

export type RemoteResult =
  | 'applied'
  | 'duplicate' // the client already had it
  | 'gap' // ops are missing before it: nothing changed, catch up first
  | 'diverged' // it doesn't apply to what should be the server's own state: reload the drawing

export class SyncEngine {
  // The seq of the newest op the server has confirmed to this client: its own acked ops and others'.
  lastSeq: number
  // Local ops the server hasn't acked yet, oldest first.
  pending: PendingOp[] = []
  // Set when the drawing can no longer be trusted to match the server's, which only a bug could cause:
  // the editor should reload it.
  diverged = false

  private readonly drawings: Drawings
  private readonly transport: Transport
  private readonly newClientOpId: () => string

  constructor(
    drawings: Drawings,
    transport: Transport,
    lastSeq: number,
    newClientOpId: () => string = () => crypto.randomUUID(),
  ) {
    this.drawings = drawings
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
    const refused = this.drawings.applyLocal(op)
    if (refused) return refused
    this.pending.push({ clientOpId: this.newClientOpId(), op, sent: false, applied: true })
    this.flush()
    return null
  }

  // The server stored a local op as `seq`. Acks for ops that aren't pending, such as a repeat, change nothing.
  ack(clientOpId: string, seq: number) {
    const index = this.pending.findIndex((pending) => pending.clientOpId === clientOpId)
    if (index === -1) return
    const [acked] = this.pending.splice(index, 1)
    // The server applied it, after everything it committed earlier, which this client has already received.
    // If it couldn't be applied here too, the two drawings differ.
    if (!acked.applied || this.drawings.applyConfirmed(acked.op)) this.diverged = true
    this.lastSeq = Math.max(this.lastSeq, seq)
  }

  // Someone else's op, committed as `seq`, arrives.
  remote(seq: number, op: object): RemoteResult {
    if (seq <= this.lastSeq) return 'duplicate'
    if (seq !== this.lastSeq + 1) return 'gap'
    if (this.drawings.applyConfirmed(op)) return this.diverge()
    this.lastSeq = seq

    if (this.synced) {
      // Nothing to rebase: the local drawing is the confirmed one.
      if (this.drawings.applyLocal(op)) return this.diverge()
    } else {
      this.rebuildLocal()
    }
    return 'applied'
  }

  // The server refused a local op: drop it, and take it out of what the user sees. Returns the op, so the
  // user can be told which edit it was; null for one that isn't pending, such as a repeat.
  reject(clientOpId: string): object | null {
    const index = this.pending.findIndex((pending) => pending.clientOpId === clientOpId)
    if (index === -1) return null
    const [rejected] = this.pending.splice(index, 1)
    // Ops after it were applied on top of it, so rebuild rather than try to lift it out. One a rebase had
    // already left out isn't showing, so there's nothing to take out.
    if (rejected.applied) this.rebuildLocal()
    return rejected.op
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

  // The local drawing again: the confirmed one with every pending op that still applies, in order.
  private rebuildLocal() {
    this.drawings.resetLocal()
    for (const pending of this.pending) pending.applied = this.drawings.applyLocal(pending.op) === null
  }

  private diverge(): RemoteResult {
    this.diverged = true
    return 'diverged'
  }
}
