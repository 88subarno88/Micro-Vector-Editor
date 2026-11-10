import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest'
import { colorFor, latestEvery } from './cursors'
import type { Point } from './tools'

beforeEach(() => vi.useFakeTimers())
afterEach(() => vi.useRealTimers())

describe('latestEvery', () => {
  it('sends the first position at once, then only the latest once per interval', () => {
    const sent: Point[] = []
    const cursor = latestEvery((point) => sent.push(point), 100)

    cursor.to({ x: 1, y: 1 })
    cursor.to({ x: 2, y: 2 })
    cursor.to({ x: 3, y: 3 })
    expect(sent).toEqual([{ x: 1, y: 1 }])

    vi.advanceTimersByTime(100)
    expect(sent).toEqual([{ x: 1, y: 1 }, { x: 3, y: 3 }]) // where the pointer stopped gets through

    vi.advanceTimersByTime(100) // nothing new: nothing sent, and the next move goes at once
    cursor.to({ x: 4, y: 4 })
    expect(sent).toHaveLength(3)
  })

  it('never sends more than once per interval, however fast the pointer moves', () => {
    const sentAt: number[] = []
    const cursor = latestEvery(() => sentAt.push(Date.now()), 100)
    for (let ms = 0; ms < 1000; ms += 5) {
      cursor.to({ x: ms, y: 0 })
      vi.advanceTimersByTime(5)
    }
    const gaps = sentAt.slice(1).map((at, i) => at - sentAt[i])
    expect(gaps.length).toBeGreaterThanOrEqual(9)
    expect(Math.min(...gaps)).toBeGreaterThanOrEqual(100)
  })

  it('sends nothing more once stopped', () => {
    const sent: Point[] = []
    const cursor = latestEvery((point) => sent.push(point), 100)
    cursor.to({ x: 1, y: 1 })
    cursor.to({ x: 2, y: 2 })
    cursor.stop()
    vi.advanceTimersByTime(1000)
    expect(sent).toHaveLength(1)
  })
})

describe('colorFor', () => {
  it('gives a user the same colour every time, and different users a spread of colours', () => {
    expect(colorFor('u1')).toBe(colorFor('u1'))
    const spread = new Set(Array.from({ length: 40 }, (_, i) => colorFor(`user-${i}`)))
    expect(spread.size).toBeGreaterThan(4)
  })
})
