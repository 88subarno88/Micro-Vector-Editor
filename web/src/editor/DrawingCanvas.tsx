import { useEffect, useRef, type PointerEvent } from 'react'
import { paint, type DrawCommand } from './paint'
import type { Point } from './tools'

// The size of every drawing, as the core saves it.
export const drawingWidth = 800
export const drawingHeight = 600

export type PointerPhase = 'down' | 'move' | 'up' | 'cancel'

// Shows a drawing. Sharp on high-density screens, and white like paper even in dark mode, since a
// drawing's colours are its own. Reports pointer events in drawing coordinates, whatever size the canvas
// is shown at.
export default function DrawingCanvas({ commands, onPointer }: {
  commands: DrawCommand[]
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
  }, [commands])

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
