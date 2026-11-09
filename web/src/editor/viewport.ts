// Zoom and pan. The viewport is how the canvas looks at the drawing, kept in the editor's own state: it isn't
// part of the document, and every viewer has their own.

import type { Point } from './tools'

export interface Viewport {
  scale: number // screen units per drawing unit
  x: number // the drawing point at the canvas's top left
  y: number
}

export const fitted: Viewport = { scale: 1, x: 0, y: 0 }
export const minimumScale = 0.25
export const maximumScale = 8
// Each zoom step in or out, by button, key or wheel notch.
export const zoomStep = 1.25

// Screen (canvas units, before zoom) to drawing, and back.
export const toDrawing = (view: Viewport, screen: Point): Point => ({
  x: screen.x / view.scale + view.x,
  y: screen.y / view.scale + view.y,
})

export const toScreen = (view: Viewport, drawing: Point): Point => ({
  x: (drawing.x - view.x) * view.scale,
  y: (drawing.y - view.y) * view.scale,
})

// Zooms by `factor` around a point on the screen, which stays over the same part of the drawing: the way
// zooming toward the pointer feels. The scale stays within its limits.
export function zoomAround(view: Viewport, factor: number, screen: Point): Viewport {
  const scale = Math.min(maximumScale, Math.max(minimumScale, view.scale * factor))
  const anchor = toDrawing(view, screen)
  return { scale, x: anchor.x - screen.x / scale, y: anchor.y - screen.y / scale }
}

// Moves the view by a distance on the screen, as a scroll or trackpad gesture does.
export const panBy = (view: Viewport, dx: number, dy: number): Viewport => ({
  ...view,
  x: view.x + dx / view.scale,
  y: view.y + dy / view.scale,
})
