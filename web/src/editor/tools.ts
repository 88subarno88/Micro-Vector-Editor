// The drawing tools: how a drag on the canvas becomes an addShape op. The sizes follow the desktop app.

export type DrawingTool = 'rect' | 'roundedRect' | 'circle' | 'line'

export const drawingTools: { tool: DrawingTool; label: string }[] = [
  { tool: 'rect', label: 'Rectangle' },
  { tool: 'roundedRect', label: 'Rounded rectangle' },
  { tool: 'circle', label: 'Circle' },
  { tool: 'line', label: 'Line' },
]

export interface Point {
  x: number
  y: number
}

// The core's defaults for a new shape: a thin black outline on white.
export const defaultStyle = { stroke: 'black', fill: 'white', strokeWidth: 1 }

const roundedCornerRadius = 20
// A drag shorter than this, in drawing units, is a click, not a shape.
const minimumDrag = 2

// A new shape ID in the core's own form: "s_" and 16 hex digits.
export function newShapeId(): string {
  const bytes = crypto.getRandomValues(new Uint8Array(8))
  return `s_${Array.from(bytes, (b) => b.toString(16).padStart(2, '0')).join('')}`
}

// The addShape op for dragging from `start` to `end` with `tool`, or null for a drag too small to mean a
// shape. Boxed shapes fill the box the drag spans, in whichever direction it went.
export function shapeForDrag(tool: DrawingTool, start: Point, end: Point, id: string) {
  const width = Math.abs(end.x - start.x)
  const height = Math.abs(end.y - start.y)
  if (Math.max(width, height) < minimumDrag) return null
  const x = Math.min(start.x, end.x)
  const y = Math.min(start.y, end.y)

  const shape = (() => {
    switch (tool) {
      case 'rect':
        return { kind: 'rect', x, y, width, height, cornerRadius: 0 }
      case 'roundedRect':
        return { kind: 'rect', x, y, width, height, cornerRadius: roundedCornerRadius }
      case 'circle': {
        // Grows from the corner where the drag started, sized by the drag's longer side.
        const radius = Math.max(width, height) / 2
        return { kind: 'circle', center: { x: x + radius, y: y + radius }, radius }
      }
      case 'line':
        return { kind: 'line', start, end }
    }
  })()
  return { kind: 'addShape', id, shape: { ...shape, style: defaultStyle } }
}
