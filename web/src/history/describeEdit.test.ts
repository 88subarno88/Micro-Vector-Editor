import { describe, expect, it } from 'vitest'
import { describeEdit } from './describeEdit'

describe('describeEdit', () => {
  it('names the shape added', () => {
    expect(describeEdit({ kind: 'addShape', id: 'a', shape: { kind: 'circle' } })).toBe('Added a circle')
    expect(describeEdit({ kind: 'addShape', id: 'a', shape: { kind: 'rect', cornerRadius: 0 } })).toBe('Added a rectangle')
    expect(describeEdit({ kind: 'addShape', id: 'a', shape: { kind: 'rect', cornerRadius: 8 } })).toBe('Added a rounded rectangle')
    expect(describeEdit({ kind: 'addShape', id: 'a', shape: { kind: 'text', text: 'Hi' } })).toBe('Added the text “Hi”')
    expect(describeEdit({ kind: 'addShape', id: 'a', shape: { kind: 'blob' } })).toBe('Added a shape')
  })

  it('says how far a shape moved, and how much it was resized, without long fractions', () => {
    expect(describeEdit({ kind: 'move', id: 'a', dx: 12.345, dy: -3 })).toBe('Moved a shape by 12.3, -3')
    expect(describeEdit({ kind: 'resize', id: 'a', factor: 1.5 })).toBe('Resized a shape to 150%')
    expect(describeEdit({ kind: 'deleteShape', id: 'a' })).toBe('Deleted a shape')
  })

  it('lists the parts of a style that changed', () => {
    expect(describeEdit({ kind: 'setStyle', id: 'a', fill: 'none' })).toBe("Changed a shape's style: no fill")
    expect(describeEdit({ kind: 'setStyle', id: 'a', stroke: 'red', fill: '#00ff00', strokeWidth: 2 }))
      .toBe("Changed a shape's style: stroke red, fill #00ff00, width 2")
  })

  it('still says something for an edit it does not know', () => {
    expect(describeEdit({ kind: 'teleport' })).toBe('An edit')
  })
})
