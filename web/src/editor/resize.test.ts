import { describe, expect, it } from 'vitest'
import { anchorFrom, factorFor, handleFor, minimumFactor, scaleBounds } from './resize'

const box = (x: number, y: number, width: number, height: number) => ({ x, y, width, height })

describe('anchorFrom', () => {
  it('finds the top left of a shape that grows from there, like a rectangle', () => {
    expect(anchorFrom(box(10, 20, 30, 40), box(10, 20, 60, 80), 2)).toEqual({ x: 10, y: 20 })
  })

  it('finds the centre of a shape that grows from there, like a circle', () => {
    expect(anchorFrom(box(0, 0, 20, 20), box(-10, -10, 40, 40), 2)).toEqual({ x: 10, y: 10 })
  })

  it("finds a line's start, even at the bottom right of its box", () => {
    // A line from (100, 100) back to (60, 80): its box is (60, 80) to (100, 100).
    expect(anchorFrom(box(60, 80, 40, 20), box(20, 60, 80, 40), 2)).toEqual({ x: 100, y: 100 })
  })
})

describe('handleFor', () => {
  it('puts the handle on the corner farthest from the anchor', () => {
    expect(handleFor(box(10, 20, 30, 40), { x: 10, y: 20 })).toEqual({ x: 40, y: 60 })
    expect(handleFor(box(60, 80, 40, 20), { x: 100, y: 100 })).toEqual({ x: 60, y: 80 })
  })
})

describe('factorFor', () => {
  it('scales so the handle follows the pointer along its line from the anchor', () => {
    const anchor = { x: 100, y: 100 }
    const handle = { x: 200, y: 200 }
    expect(factorFor(anchor, handle, { x: 300, y: 300 })).toBe(2)
    expect(factorFor(anchor, handle, { x: 150, y: 150 })).toBe(0.5)
    expect(factorFor(anchor, handle, { x: 300, y: 100 })).toBe(1) // across the line: halfway along it
  })

  it("can't shrink a shape to nothing or turn it inside out", () => {
    expect(factorFor({ x: 0, y: 0 }, { x: 10, y: 0 }, { x: -50, y: 0 })).toBe(minimumFactor)
  })

  it('leaves the size alone when the handle is on the anchor', () => {
    expect(factorFor({ x: 5, y: 5 }, { x: 5, y: 5 }, { x: 50, y: 50 })).toBe(1)
  })
})

describe('scaleBounds', () => {
  it('scales a box around a point', () => {
    expect(scaleBounds(box(10, 10, 20, 20), { x: 20, y: 20 }, 2)).toEqual(box(0, 0, 40, 40))
  })
})
