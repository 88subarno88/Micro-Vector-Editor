import type { Point } from './tools'

// How often this user's pointer is sent: 10 a second, the most the server takes. Any more would be dropped
// there, and the dropped one might be where the pointer came to rest.
export const cursorIntervalMs = 100

// Sends the pointer's position at most once every `intervalMs`: the first at once, then the latest one at
// the end of each interval, so the last position always gets through.
export function latestEvery(send: (point: Point) => void, intervalMs = cursorIntervalMs) {
  let waiting: Point | null = null
  let timer: ReturnType<typeof setTimeout> | null = null
  const tick = () => {
    timer = null
    if (!waiting) return
    send(waiting)
    waiting = null
    timer = setTimeout(tick, intervalMs)
  }
  return {
    to(point: Point) {
      if (timer !== null) {
        waiting = point
        return
      }
      send(point)
      timer = setTimeout(tick, intervalMs)
    },
    stop() {
      if (timer !== null) clearTimeout(timer)
      timer = null
      waiting = null
    },
  }
}

const colors = ['#d1495b', '#00798c', '#edae49', '#66a182', '#8d5a97', '#e07a5f', '#3d5a80', '#9c6644']

// A colour for each user, the same on every screen and every visit.
export function colorFor(userId: string): string {
  let hash = 0
  for (const char of userId) hash = (hash * 31 + char.charCodeAt(0)) | 0
  return colors[Math.abs(hash) % colors.length]
}
