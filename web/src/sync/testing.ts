// Stand-ins for the sync tests.
import type { SocketLike } from './SyncClient'
import type { Drawings } from './SyncEngine'

// A stand-in for a WebSocket: records what's sent, and lets the test play the server.
export class FakeSocket implements SocketLike {
  readyState = 0
  sent: unknown[] = []
  closedWith: number | null = null
  onopen: ((event: Event) => void) | null = null
  onmessage: ((event: MessageEvent) => void) | null = null
  onclose: ((event: CloseEvent) => void) | null = null
  readonly url: string
  constructor(url: string) {
    this.url = url
  }

  send(data: string) {
    this.sent.push(JSON.parse(data))
  }
  close(code = 1000) {
    this.closedWith = code
    this.readyState = 3
  }

  opened() {
    this.readyState = 1
    this.onopen?.(new Event('open'))
  }
  receive(message: unknown) {
    this.onmessage?.({ data: JSON.stringify(message) } as MessageEvent)
  }
  dropped(code: number, reason = '') {
    this.readyState = 3
    this.onclose?.({ code, reason } as CloseEvent)
  }
}

// Drawings that are just numbers: {add: n} adds n, {set: n} sets it. Anything that would go below 0 is
// refused. `confirmed` is what the server has; `local` is what the user sees. A snapshot is {value: n}.
export function counters(start = 0) {
  const step = (value: number, op: object): number | null => {
    const { add, set } = op as { add?: number; set?: number }
    const next = set ?? value + (add ?? 0)
    return next < 0 ? null : next
  }
  const drawings: Drawings & { confirmed: number; local: number } = {
    confirmed: start,
    local: start,
    applyConfirmed(op) {
      const next = step(drawings.confirmed, op)
      if (next === null) return 'would go below zero'
      drawings.confirmed = next
      return null
    },
    applyLocal(op) {
      const next = step(drawings.local, op)
      if (next === null) return 'would go below zero'
      drawings.local = next
      return null
    },
    resetLocal() {
      drawings.local = drawings.confirmed
    },
    loadConfirmed(snapshot) {
      const { value } = snapshot as { value?: number }
      if (value === undefined) return 'not a snapshot'
      drawings.confirmed = value
      return null
    },
  }
  return drawings
}
