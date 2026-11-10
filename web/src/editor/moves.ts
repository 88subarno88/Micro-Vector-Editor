import type { Point } from './tools'

// How often a dragged shape's move is sent while the pointer moves: 20 a second, inside the server's limit
// of 30 ops a second, and often enough that others see the shape travel rather than jump.
export const moveIntervalMs = 50

// Turns a drag into a few move ops rather than one per pointer event: while the pointer moves, at most one
// every `intervalMs`, each by how far it went since the last; when the drag ends, one more for the rest.
// Movements no bigger than `minimum` either way aren't sent: a click isn't a move.
export function coalescedMoves(send: (by: Point) => void, minimum: number, intervalMs = moveIntervalMs) {
  let sent: Point = { x: 0, y: 0 } // how far the ops so far have moved the shape
  let latest = sent // how far the pointer has
  let timer: ReturnType<typeof setTimeout> | null = null

  const unsent = (): Point => ({ x: latest.x - sent.x, y: latest.y - sent.y })
  const flush = () => {
    timer = null
    const by = unsent()
    if (Math.abs(by.x) <= minimum && Math.abs(by.y) <= minimum) return
    sent = latest
    send(by)
  }

  return {
    // The pointer is now `total` from where the drag started.
    to(total: Point) {
      latest = total
      timer ??= setTimeout(flush, intervalMs)
    },
    // The drag ends, at `total` or wherever the pointer last was.
    end(total = latest) {
      if (timer !== null) clearTimeout(timer)
      latest = total
      flush()
    },
    // How far the pointer has moved the shape beyond what's been sent: for drawing it there meanwhile.
    unsent,
  }
}
