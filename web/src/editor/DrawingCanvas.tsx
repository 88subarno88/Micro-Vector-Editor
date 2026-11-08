import { useEffect, useRef } from 'react'
import { paint, type DrawCommand } from './paint'

// The size of every drawing, as the core saves it.
export const drawingWidth = 800
export const drawingHeight = 600

// Shows a drawing. Sharp on high-density screens, and white like paper even in dark mode, since a
// drawing's colours are its own.
export default function DrawingCanvas({ commands }: { commands: DrawCommand[] }) {
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

  return <canvas ref={canvas} className="drawing" width={drawingWidth} height={drawingHeight} role="img" aria-label="The drawing" />
}
