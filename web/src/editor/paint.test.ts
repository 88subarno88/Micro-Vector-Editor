import { describe, expect, it } from 'vitest'
import { paint, type DrawCommand, type Painter, type Style } from './paint'

// Records every call and the style in force when it was made.
function recorder() {
  const calls: string[] = []
  const state = { fillStyle: '', strokeStyle: '', lineWidth: 0, lineJoin: '', font: '', textBaseline: '' }
  const record = (name: string) => (...args: unknown[]) => {
    calls.push(`${name}(${args.map((a) => (typeof a === 'number' ? Math.round(a * 100) / 100 : a)).join(', ')})`)
  }
  const painter = new Proxy(state, {
    get: (target, key: string) => (key in target ? target[key as keyof typeof state] : record(key)),
    set: (target, key: string, value) => {
      ;(target as Record<string, unknown>)[key] = value
      return true
    },
  }) as unknown as Painter
  return { painter, calls, state }
}

const style: Style = { stroke: 'red', fill: '#00ff00', strokeWidth: 2 }
// A command without its id, and optionally without its style: Omit applied to each kind of command.
type Shape = { [K in DrawCommand['kind']]: Omit<Extract<DrawCommand, { kind: K }>, 'id' | 'style'> }[DrawCommand['kind']]
const draw = (command: Shape & { style?: Style }) => {
  const r = recorder()
  paint(r.painter, [{ id: 's', style, ...command } as DrawCommand])
  return r
}

describe('paint', () => {
  it('draws a rectangle, rounding its corners only when it has a radius', () => {
    expect(draw({ kind: 'rect', x: 1, y: 2, width: 3, height: 4, cornerRadius: 0 }).calls).toEqual([
      'beginPath()', 'rect(1, 2, 3, 4)', 'fill()', 'stroke()',
    ])
    expect(draw({ kind: 'rect', x: 1, y: 2, width: 3, height: 4, cornerRadius: 5 }).calls).toContain('roundRect(1, 2, 3, 4, 5)')
  })

  it('uses the style, with square corners on outlines', () => {
    const { state } = draw({ kind: 'rect', x: 0, y: 0, width: 1, height: 1, cornerRadius: 0 })
    expect(state).toMatchObject({ fillStyle: '#00ff00', strokeStyle: 'red', lineWidth: 2, lineJoin: 'miter' })
  })

  it('leaves out the fill when it is "none", and the outline when its width is 0', () => {
    const unfilled = draw({ kind: 'ellipse', cx: 0, cy: 0, rx: 1, ry: 1, style: { ...style, fill: 'none' } })
    expect(unfilled.calls).not.toContain('fill()')
    expect(unfilled.calls).toContain('stroke()')

    const unstroked = draw({ kind: 'ellipse', cx: 0, cy: 0, rx: 1, ry: 1, style: { ...style, strokeWidth: 0 } })
    expect(unstroked.calls).toContain('fill()')
    expect(unstroked.calls).not.toContain('stroke()')
  })

  it('draws an ellipse all the way round', () => {
    expect(draw({ kind: 'ellipse', cx: 10, cy: 20, rx: 30, ry: 40 }).calls).toContain('ellipse(10, 20, 30, 40, 0, 0, 6.28)')
  })

  it('fills a closed polyline but never an open one', () => {
    const closed = draw({ kind: 'polyline', points: [0, 0, 10, 0, 5, 8], closed: true })
    expect(closed.calls).toEqual(['beginPath()', 'moveTo(0, 0)', 'lineTo(10, 0)', 'lineTo(5, 8)', 'closePath()', 'fill()', 'stroke()'])

    const open = draw({ kind: 'polyline', points: [0, 0, 10, 0], closed: false })
    expect(open.calls).toEqual(['beginPath()', 'moveTo(0, 0)', 'lineTo(10, 0)', 'stroke()'])
  })

  it('skips a polyline without a point', () => {
    expect(draw({ kind: 'polyline', points: [], closed: false }).calls).toEqual(['beginPath()'])
  })

  it('places text by its top left, filled and then outlined', () => {
    const { calls, state } = draw({ kind: 'text', x: 5, y: 6, text: 'Hi <&>', fontFamily: 'Arial', fontSize: 24 })
    expect(calls).toEqual(['beginPath()', 'fillText(Hi <&>, 5, 6)', 'strokeText(Hi <&>, 5, 6)'])
    expect(state).toMatchObject({ font: '24px Arial, sans-serif', textBaseline: 'top', lineJoin: 'bevel' })
  })

  it('draws commands in order, bottom to top', () => {
    const r = recorder()
    paint(r.painter, [
      { id: 'a', style, kind: 'rect', x: 0, y: 0, width: 1, height: 1, cornerRadius: 0 },
      { id: 'b', style, kind: 'ellipse', cx: 0, cy: 0, rx: 1, ry: 1 },
    ])
    expect(r.calls.filter((c) => c.startsWith('rect') || c.startsWith('ellipse'))).toEqual(['rect(0, 0, 1, 1)', 'ellipse(0, 0, 1, 1, 0, 0, 6.28)'])
  })
})
