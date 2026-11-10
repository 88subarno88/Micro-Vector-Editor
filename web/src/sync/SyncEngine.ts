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
//
// When the connection drops, answers can be lost with it. On rejoining, the server sends what was missed,
// which may include this client's own ops: each carries its clientOpId, so it's taken as the lost ack. Then
// every op still pending is sent again with the same clientOpId; the server recognises any it already stored
// and acks it with the original seq instead of applying it twice. A lost reject can't be recovered: that op
// is sent again and the server decides afresh, after what came since.

export interface Drawings {
  // Each applies an op: null, or why it doesn't apply (and nothing changed).
  applyConfirmed(op: object): string | null
  applyLocal(op: object): string | null
  // Makes the local drawing a copy of the confirmed one.
  resetLocal(): void
  // Replaces the confirmed drawing with the server's whole drawing: null, or why it couldn't be loaded.
  loadConfirmed(snapshot: object): string | null
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
    if (seq <= this.lastSeq) {
      // A resent op the server had already stored, before the snapshot this client restarted from: the
      // confirmed drawing has it, so only the local one, which applied it again, needs rebuilding.
      this.rebuildLocal()
      return
    }
    // The server applied it, after everything it committed earlier, which this client has already received.
    // If it couldn't be applied here too, the two drawings differ.
    if (this.drawings.applyConfirmed(acked.op) || (index === 0 && !acked.applied)) this.diverged = true
    this.lastSeq = seq
    // Acked ahead of older pending ops, which only happens when their rejects were lost with a connection:
    // the server's order isn't the local one, so the local drawing follows the server's.
    if (index > 0) this.rebuildLocal()
  }

  // An op committed as `seq` arrives: someone else's, or one of this client's own whose ack was lost, which
  // its clientOpId tells apart.
  remote(seq: number, op: object, clientOpId = ''): RemoteResult {
    if (seq <= this.lastSeq) return 'duplicate'
    if (seq !== this.lastSeq + 1) return 'gap'
    if (this.pending.some((pending) => pending.clientOpId === clientOpId)) {
      this.ack(clientOpId, seq)
      return this.diverged ? 'diverged' : 'applied'
    }
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

  // Starts again from the server's whole drawing at `seq`, as when rejoining too far behind for the server to
  // send what was missed. Pending ops stay on top, to be resent; any the drawing already holds is shown
  // twice until its ack comes back with an older seq.
  loadSnapshot(seq: number, snapshot: object) {
    if (this.drawings.loadConfirmed(snapshot)) {
      this.diverged = true
      return
    }
    this.lastSeq = seq
    this.rebuildLocal()
  }

  // Sends every pending op again, oldest first, as after reconnecting: whatever was sent before may have
  // been lost with the connection.
  resend() {
    for (const pending of this.pending) pending.sent = false
    this.flush()
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
