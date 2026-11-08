import { describe, expect, it } from 'vitest'
import { defaultStyle, newShapeId, shapeForDrag } from './tools'

const shape = (...args: Parameters<typeof shapeForDrag>) => shapeForDrag(...args)?.shape

describe('shapeForDrag', () => {
  it('fills the dragged box with a rectangle, whichever way the drag went', () => {
    const expected = { kind: 'rect', x: 10, y: 20, width: 30, height: 40, cornerRadius: 0, style: defaultStyle }
    expect(shape('rect', { x: 10, y: 20 }, { x: 40, y: 60 }, 'a')).toEqual(expected)
    expect(shape('rect', { x: 40, y: 60 }, { x: 10, y: 20 }, 'a')).toEqual(expected)
    expect(shape('rect', { x: 40, y: 20 }, { x: 10, y: 60 }, 'a')).toEqual(expected)
  })

  it('rounds a rounded rectangle by 20, as on the desktop', () => {
    expect(shape('roundedRect', { x: 0, y: 0 }, { x: 100, y: 50 }, 'a')).toMatchObject({ kind: 'rect', cornerRadius: 20 })
  })

  it("sizes a circle by the drag's longer side, from its top left", () => {
    expect(shape('circle', { x: 10, y: 10 }, { x: 110, y: 50 }, 'a')).toMatchObject({
      kind: 'circle', center: { x: 60, y: 60 }, radius: 50,
    })
  })

  it('draws a line from where the drag started to where it ended', () => {
    expect(shape('line', { x: 5, y: 6 }, { x: 1, y: 2 }, 'a')).toMatchObject({ kind: 'line', start: { x: 5, y: 6 }, end: { x: 1, y: 2 } })
  })

  it('treats a tiny drag as a click, not a shape', () => {
    expect(shapeForDrag('rect', { x: 10, y: 10 }, { x: 11, y: 11.5 }, 'a')).toBeNull()
    expect(shapeForDrag('line', { x: 10, y: 10 }, { x: 10, y: 12 }, 'a')).not.toBeNull()
  })

  it('makes an addShape op with the given ID and the default style', () => {
    expect(shapeForDrag('rect', { x: 0, y: 0 }, { x: 5, y: 5 }, 's_1')).toMatchObject({
      kind: 'addShape', id: 's_1', shape: { style: { stroke: 'black', fill: 'white', strokeWidth: 1 } },
    })
  })
})

describe('newShapeId', () => {
  it("looks like the core's own IDs, and doesn't repeat", () => {
    const ids = new Set(Array.from({ length: 1000 }, newShapeId))
    expect(ids.size).toBe(1000)
    for (const id of ids) expect(id).toMatch(/^s_[0-9a-f]{16}$/)
  })
})
