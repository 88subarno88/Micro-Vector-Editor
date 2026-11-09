// The drawing tools: how a drag or click on the canvas becomes an addShape op. Sizes follow the desktop app.

// Tools that draw a shape filling the box a drag spans.
export type BoxTool = 'rect' | 'roundedRect' | 'circle' | 'line' | 'hexagon'
export type DrawingTool = BoxTool | 'freehand' | 'text'
// Select picks shapes to move and delete; the rest draw.
export type Tool = 'select' | DrawingTool

export const tools: { tool: Tool; label: string }[] = [
  { tool: 'select', label: 'Select' },
  { tool: 'rect', label: 'Rectangle' },
  { tool: 'roundedRect', label: 'Rounded rectangle' },
  { tool: 'circle', label: 'Circle' },
  { tool: 'line', label: 'Line' },
  { tool: 'hexagon', label: 'Hexagon' },
  { tool: 'freehand', label: 'Freehand' },
  { tool: 'text', label: 'Text' },
]

export interface Point {
  x: number
  y: number
}

// The core's defaults for a new shape: a thin black outline on white.
export const defaultStyle = { stroke: 'black', fill: 'white', strokeWidth: 1 }

const roundedCornerRadius = 20
export const defaultFontSize = 16
// A drag shorter than this, in drawing units, is a click, not a shape.
const minimumDrag = 2

// A new shape ID in the core's own form: "s_" and 16 hex digits.
export function newShapeId(): string {
  const bytes = crypto.getRandomValues(new Uint8Array(8))
  return `s_${Array.from(bytes, (b) => b.toString(16).padStart(2, '0')).join('')}`
}

// The addShape op for dragging from `start` to `end` with `tool`, or null for a drag too small to mean a
// shape. Boxed shapes fill the box the drag spans, in whichever direction it went.
export function shapeForDrag(tool: BoxTool, start: Point, end: Point, id: string) {
  const width = Math.abs(end.x - start.x)
  const height = Math.abs(end.y - start.y)
  if (Math.max(width, height) < minimumDrag) return null
  // A hexagon fits the box's shorter side, so a thin drag would make one too small to see.
  if (tool === 'hexagon' && Math.min(width, height) < minimumDrag) return null
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
      case 'hexagon':
        // Centred in the box, as large as fits: half its shorter side.
        return { kind: 'hexagon', center: { x: x + width / 2, y: y + height / 2 }, radius: Math.min(width, height) / 2 }
    }
  })()
  return addShape(id, shape)
}

// The addShape op for a freehand stroke through `points`, or null for one too small to be a stroke.
export function shapeForStroke(points: Point[], id: string) {
  const xs = points.map((p) => p.x)
  const ys = points.map((p) => p.y)
  const size = Math.max(Math.max(...xs) - Math.min(...xs), Math.max(...ys) - Math.min(...ys))
  if (points.length < 2 || size < minimumDrag) return null
  return addShape(id, { kind: 'freehand', points })
}

// The addShape op for text whose top left is at `at`, or null for empty text.
export function textShape(at: Point, text: string, id: string) {
  if (!text.trim()) return null
  return addShape(id, { kind: 'text', position: at, text, fontSize: defaultFontSize })
}

function addShape(id: string, shape: object) {
  return { kind: 'addShape', id, shape: { ...shape, style: defaultStyle } }
}
