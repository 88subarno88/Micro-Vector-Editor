// Draws what the core's Document.drawCommands() describes onto a 2D canvas. The core decides the geometry
// (a hexagon's corners, where text starts), exactly as it does for the desktop app's Qt renderer; this only
// replays four primitives, following the same rules as that renderer.

export interface Style {
  stroke: string
  fill: string // "none" for no fill
  strokeWidth: number
}

export type DrawCommand = { id: string; style: Style } & (
  | { kind: 'rect'; x: number; y: number; width: number; height: number; cornerRadius: number }
  | { kind: 'ellipse'; cx: number; cy: number; rx: number; ry: number }
  | { kind: 'polyline'; points: number[]; closed: boolean }
  | { kind: 'text'; x: number; y: number; text: string; fontFamily: string; fontSize: number }
)

// The subset of CanvasRenderingContext2D used here, so tests can record the calls.
export type Painter = Pick<
  CanvasRenderingContext2D,
  | 'beginPath' | 'rect' | 'roundRect' | 'ellipse' | 'moveTo' | 'lineTo' | 'closePath' | 'fill' | 'stroke'
  | 'fillText' | 'strokeText' | 'fillStyle' | 'strokeStyle' | 'lineWidth' | 'lineJoin' | 'font' | 'textBaseline'
>

const hasFill = (style: Style) => style.fill !== 'none'
// A width of 0 means no outline, as in SVG.
const hasStroke = (style: Style) => style.stroke !== 'none' && style.strokeWidth > 0

function finish(painter: Painter, style: Style, fill: boolean) {
  if (fill && hasFill(style)) painter.fill()
  if (hasStroke(style)) painter.stroke()
}

export function paint(painter: Painter, commands: DrawCommand[]) {
  for (const command of commands) {
    const { style } = command
    painter.fillStyle = style.fill
    painter.strokeStyle = style.stroke
    painter.lineWidth = style.strokeWidth
    painter.lineJoin = 'miter' // square corners on thick outlines, as on the desktop
    painter.beginPath()

    switch (command.kind) {
      case 'rect':
        if (command.cornerRadius > 0) {
          painter.roundRect(command.x, command.y, command.width, command.height, command.cornerRadius)
        } else {
          painter.rect(command.x, command.y, command.width, command.height)
        }
        finish(painter, style, true)
        break
      case 'ellipse':
        painter.ellipse(command.cx, command.cy, command.rx, command.ry, 0, 0, 2 * Math.PI)
        finish(painter, style, true)
        break
      case 'polyline': {
        const { points } = command
        if (points.length < 2) break
        painter.moveTo(points[0], points[1])
        for (let i = 2; i + 1 < points.length; i += 2) painter.lineTo(points[i], points[i + 1])
        if (command.closed) painter.closePath()
        finish(painter, style, command.closed) // an open line is never filled
        break
      }
      case 'text':
        painter.font = `${command.fontSize}px ${command.fontFamily}, sans-serif`
        painter.textBaseline = 'top' // the core places text by its top left
        painter.lineJoin = 'bevel' // mitred joins spike on the sharp corners of glyphs
        if (hasFill(style)) painter.fillText(command.text, command.x, command.y)
        if (hasStroke(style)) painter.strokeText(command.text, command.x, command.y)
        break
    }
  }
}
