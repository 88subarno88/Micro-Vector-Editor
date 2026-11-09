import { useEffect, useRef, type PointerEvent } from 'react'
import { paint, type DrawCommand } from './paint'
import type { Point } from './tools'
import type { Bounds } from './useDrawing'

// The size of every drawing, as the core saves it.
export const drawingWidth = 800
export const drawingHeight = 600

export type PointerPhase = 'down' | 'move' | 'up' | 'cancel'

// Shows a drawing. Sharp on high-density screens, and white like paper even in dark mode, since a
// drawing's colours are its own. Reports pointer events in drawing coordinates, whatever size the canvas
// is shown at.
// Clear of the shape's own outline.
const outlinePadding = 4
// The resize handle's size, in drawing units.
export const handleSize = 10

// `outline`, if set, is the box around the selected shape, drawn dashed over the drawing; `handle` is
// where its resize handle goes.
export default function DrawingCanvas({ commands, outline, handle, onPointer }: {
  commands: DrawCommand[]
  outline?: Bounds | null
  handle?: Point | null
  onPointer?: (phase: PointerPhase, point: Point) => void
}) {
  const canvas = useRef<HTMLCanvasElement>(null)

  useEffect(() => {
    const context = canvas.current?.getContext('2d')
    if (!canvas.current || !context) return
    const scale = window.devicePixelRatio || 1
    canvas.current.width = drawingWidth * scale
    canvas.current.height = drawingHeight * scale
    context.setTransform(scale, 0, 0, scale, 0, 0)
    context.fillStyle = '#ffffff'
    context.fillRect(0, 0, drawingWidth, drawingHeight)
    paint(context, commands)
    if (outline) {
      context.setLineDash([6, 4])
      context.lineWidth = 1
      context.strokeStyle = '#1f6feb'
      context.strokeRect(
        outline.x - outlinePadding,
        outline.y - outlinePadding,
        outline.width + 2 * outlinePadding,
        outline.height + 2 * outlinePadding,
      )
      context.setLineDash([])
    }
    if (handle) {
      context.fillStyle = '#ffffff'
      context.fillRect(handle.x - handleSize / 2, handle.y - handleSize / 2, handleSize, handleSize)
      context.strokeRect(handle.x - handleSize / 2, handle.y - handleSize / 2, handleSize, handleSize)
    }
  }, [commands, outline, handle])

  const report = (phase: PointerPhase) => (event: PointerEvent<HTMLCanvasElement>) => {
    if (!onPointer) return
    const box = event.currentTarget.getBoundingClientRect()
    const x = (event.clientX - box.left) * (box.width ? drawingWidth / box.width : 1)
    const y = (event.clientY - box.top) * (box.height ? drawingHeight / box.height : 1)
    // Keep receiving moves while a drag leaves the canvas.
    if (phase === 'down') event.currentTarget.setPointerCapture?.(event.pointerId)
    onPointer(phase, { x, y })
  }

  return (
    <canvas
      ref={canvas}
      className="drawing"
      width={drawingWidth}
      height={drawingHeight}
      role="img"
      aria-label="The drawing"
      onPointerDown={report('down')}
      onPointerMove={report('move')}
      onPointerUp={report('up')}
      onPointerCancel={report('cancel')}
    />
  )
}
