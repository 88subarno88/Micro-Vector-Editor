import { describe, expect, it } from 'vitest'
import { restoreOps, type ShapeEntry } from './restore'

const style = (fill = 'white') => ({ stroke: 'black', fill, strokeWidth: 1 })
const rect = (id: string, x = 0, fill?: string): ShapeEntry => ({ id, shape: { kind: 'rect', x, style: style(fill) } })

describe('restoreOps', () => {
  it('needs nothing when the drawing is already the version', () => {
    expect(restoreOps([rect('a'), rect('b')], [rect('a'), rect('b')])).toEqual([])
  })

  it('deletes what was added since, and adds back what was deleted, where it was', () => {
    expect(restoreOps([rect('a'), rect('c')], [rect('a'), rect('b'), rect('c')])).toEqual([
      { kind: 'addShape', id: 'b', shape: rect('b').shape, position: 1 },
    ])
    expect(restoreOps([rect('a'), rect('new')], [rect('a')])).toEqual([{ kind: 'deleteShape', id: 'new' }])
  })

  it('restores a style with one setStyle, and anything else by replacing the shape', () => {
    expect(restoreOps([rect('a', 0, 'red')], [rect('a', 0, 'white')])).toEqual([
      { kind: 'setStyle', id: 'a', ...style('white') },
    ])
    expect(restoreOps([rect('a', 50)], [rect('a', 0)])).toEqual([
      { kind: 'deleteShape', id: 'a' },
      { kind: 'addShape', id: 'a', shape: rect('a', 0).shape, position: 0 },
    ])
  })

  it('puts shapes back in their drawing order', () => {
    // b was brought to the top since; the version had it at the bottom.
    expect(restoreOps([rect('a'), rect('c'), rect('b')], [rect('b'), rect('a'), rect('c')])).toEqual([
      { kind: 'deleteShape', id: 'b' },
      { kind: 'addShape', id: 'b', shape: rect('b').shape, position: 0 },
    ])
  })
})
