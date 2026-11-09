import { describe, expect, it } from 'vitest'
import { fitted, maximumScale, minimumScale, panBy, toDrawing, toScreen, zoomAround } from './viewport'

describe('the viewport', () => {
  it('shows the drawing as it is when fitted', () => {
    expect(toDrawing(fitted, { x: 10, y: 20 })).toEqual({ x: 10, y: 20 })
  })

  it('converts between screen and drawing both ways', () => {
    const view = { scale: 2, x: 50, y: 30 }
    expect(toDrawing(view, { x: 100, y: 100 })).toEqual({ x: 100, y: 80 })
    expect(toScreen(view, { x: 100, y: 80 })).toEqual({ x: 100, y: 100 })
  })

  it('zooms around a point, which stays over the same part of the drawing', () => {
    const pointer = { x: 300, y: 200 }
    const before = toDrawing(fitted, pointer)
    const zoomed = zoomAround(fitted, 2, pointer)
    expect(zoomed.scale).toBe(2)
    expect(toDrawing(zoomed, pointer)).toEqual(before)
  })

  it('keeps the zoom within its limits', () => {
    expect(zoomAround(fitted, 1000, { x: 0, y: 0 }).scale).toBe(maximumScale)
    expect(zoomAround(fitted, 0.001, { x: 0, y: 0 }).scale).toBe(minimumScale)
  })

  it('pans by a distance on the screen, whatever the zoom', () => {
    expect(panBy({ scale: 2, x: 0, y: 0 }, 100, -40)).toEqual({ scale: 2, x: 50, y: -20 })
  })
})
