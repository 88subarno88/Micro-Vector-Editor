// Resizing by a handle. The core's resize op scales a shape by one factor around a point of its own (the
// top left of a rectangle, the centre of a circle, the start of a line, ...). Rather than repeat that
// table here, the editor measures the point from the core: see anchorFrom.

import type { Point } from './tools'
import type { Bounds } from './useDrawing'

// The smallest factor a drag can scale by, so a shape can't be flattened to nothing.
export const minimumFactor = 0.05

// The point a shape scales around, from its box before and after scaling by `factor`. A scale by f around
// A takes each corner x to A + f(x - A), so A = (f·x - x') / (f - 1).
export function anchorFrom(before: Bounds, after: Bounds, factor: number): Point {
  return {
    x: (factor * before.x - after.x) / (factor - 1),
    y: (factor * before.y - after.y) / (factor - 1),
  }
}

// Where to put the handle: the corner of the box farthest from the anchor, so it's never on the anchor
// itself, where dragging couldn't scale anything.
export function handleFor(bounds: Bounds, anchor: Point): Point {
  const corners = [
    { x: bounds.x, y: bounds.y },
    { x: bounds.x + bounds.width, y: bounds.y },
    { x: bounds.x, y: bounds.y + bounds.height },
    { x: bounds.x + bounds.width, y: bounds.y + bounds.height },
  ]
  const distance = (p: Point) => (p.x - anchor.x) ** 2 + (p.y - anchor.y) ** 2
  return corners.reduce((far, corner) => (distance(corner) > distance(far) ? corner : far))
}

// The factor that brings the handle closest to `pointer`: the pointer's position along the line from the
// anchor through the handle, as a fraction of the handle's. Never below minimumFactor.
export function factorFor(anchor: Point, handle: Point, pointer: Point): number {
  const along = { x: handle.x - anchor.x, y: handle.y - anchor.y }
  const length = along.x ** 2 + along.y ** 2
  if (length === 0) return 1
  const factor = ((pointer.x - anchor.x) * along.x + (pointer.y - anchor.y) * along.y) / length
  return Math.max(factor, minimumFactor)
}

// A box scaled by `factor` around `anchor`.
export function scaleBounds(bounds: Bounds, anchor: Point, factor: number): Bounds {
  return {
    x: anchor.x + factor * (bounds.x - anchor.x),
    y: anchor.y + factor * (bounds.y - anchor.y),
    width: bounds.width * factor,
    height: bounds.height * factor,
  }
}
