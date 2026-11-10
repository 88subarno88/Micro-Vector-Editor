import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest'
import { coalescedMoves } from './moves'
import type { Point } from './tools'

function setUp() {
  const sent: Point[] = []
  const moves = coalescedMoves((by) => sent.push(by), 0.01, 50)
  return { sent, moves }
}

beforeEach(() => vi.useFakeTimers())
afterEach(() => vi.useRealTimers())

describe('coalescedMoves', () => {
  it('sends at most one move every interval, each by how far the pointer went since the last', () => {
    const { sent, moves } = setUp()
    moves.to({ x: 1, y: 0 })
    moves.to({ x: 2, y: 1 })
    moves.to({ x: 3, y: 1 })
    expect(sent).toEqual([]) // nothing yet: waiting for the interval

    vi.advanceTimersByTime(50)
    expect(sent).toEqual([{ x: 3, y: 1 }])

    moves.to({ x: 10, y: 4 })
    vi.advanceTimersByTime(49)
    expect(sent).toHaveLength(1)
    vi.advanceTimersByTime(1)
    expect(sent).toEqual([{ x: 3, y: 1 }, { x: 7, y: 3 }])
  })

  it('sends the rest when the drag ends, so the moves add up to the whole drag', () => {
    const { sent, moves } = setUp()
    moves.to({ x: 5, y: 5 })
    vi.advanceTimersByTime(50)
    moves.to({ x: 8, y: 2 })

    moves.end({ x: 9, y: 2 })

    expect(sent).toEqual([{ x: 5, y: 5 }, { x: 4, y: -3 }])
    vi.advanceTimersByTime(1000)
    expect(sent).toHaveLength(2) // the waiting move was folded into the last, not sent again
  })

  it('ends where the pointer last was, when no end point is given', () => {
    const { sent, moves } = setUp()
    moves.to({ x: 6, y: 1 })
    moves.end()
    expect(sent).toEqual([{ x: 6, y: 1 }])
  })

  it('sends nothing while the pointer is still, or for a click', () => {
    const { sent, moves } = setUp()
    moves.to({ x: 2, y: 0 })
    vi.advanceTimersByTime(50)
    moves.to({ x: 2, y: 0 }) // no further
    vi.advanceTimersByTime(50)
    expect(sent).toEqual([{ x: 2, y: 0 }])

    const click = setUp()
    click.moves.to({ x: 0.005, y: 0 })
    click.moves.end()
    expect(click.sent).toEqual([])
  })

  it("keeps a movement too small to send, and sends it once it's grown", () => {
    const { sent, moves } = setUp()
    moves.to({ x: 0.006, y: 0 })
    vi.advanceTimersByTime(50)
    moves.to({ x: 0.012, y: 0 })
    vi.advanceTimersByTime(50)
    expect(sent).toEqual([{ x: 0.012, y: 0 }])
  })

  it('says how far the pointer is beyond what has been sent', () => {
    const { moves } = setUp()
    moves.to({ x: 4, y: 4 })
    vi.advanceTimersByTime(50)
    moves.to({ x: 6, y: 3 })
    expect(moves.unsent()).toEqual({ x: 2, y: -1 })
  })
})
