// An edit in words, for the history: "Added a circle", "Moved a shape by 12, -3".

const shapeNames: Record<string, string> = {
  rect: 'a rectangle',
  circle: 'a circle',
  line: 'a line',
  hexagon: 'a hexagon',
  freehand: 'a freehand stroke',
}

// To one decimal place at most: drags give long fractions nobody needs to read.
const short = (value: unknown) => String(Math.round(Number(value) * 10) / 10)

export function describeEdit(op: Record<string, unknown>): string {
  switch (op.kind) {
    case 'addShape': {
      const shape = (op.shape ?? {}) as { kind?: string; cornerRadius?: number; text?: string }
      if (shape.kind === 'text') return `Added the text “${shape.text ?? ''}”`
      if (shape.kind === 'rect' && (shape.cornerRadius ?? 0) > 0) return 'Added a rounded rectangle'
      return `Added ${shapeNames[shape.kind ?? ''] ?? 'a shape'}`
    }
    case 'deleteShape':
      return 'Deleted a shape'
    case 'move':
      return `Moved a shape by ${short(op.dx)}, ${short(op.dy)}`
    case 'resize':
      return `Resized a shape to ${Math.round(Number(op.factor) * 100)}%`
    case 'setStyle': {
      const changes = [
        'stroke' in op && `stroke ${op.stroke}`,
        'fill' in op && (op.fill === 'none' ? 'no fill' : `fill ${op.fill}`),
        'strokeWidth' in op && `width ${op.strokeWidth}`,
      ].filter(Boolean)
      return `Changed a shape's style: ${changes.join(', ')}`
    }
  }
  return 'An edit'
}
