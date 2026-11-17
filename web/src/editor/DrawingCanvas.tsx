import { useEffect, useRef, useState, type PointerEvent } from 'react'
import { paint, type DrawCommand } from './paint'
import type { Point } from './tools'
import type { Bounds } from './useDrawing'
import { fitted, panBy, toDrawing, zoomAround, zoomStep, type Viewport } from './viewport'

// The size of every drawing, as the core saves it, and of the canvas showing it.
export const drawingWidth = 800
export const drawingHeight = 600

export type PointerPhase = 'down' | 'move' | 'up' | 'cancel'

// On-screen sizes, kept the same at any zoom.
const outlinePadding = 4
export const handleSize = 10
// A wheel's lines and pages, in pixels.
const pixelsPerLine = 16
const pixelsPerPage = drawingHeight
// The selection's colour: the app's accent.
const selectionColor = '#6366f1'
// The most pixels per drawing unit the canvas keeps, which bounds its memory however large it's shown.
const maxDensity = 4

// Shows a drawing through a viewport, as a sheet of white paper; around it, when zoomed out, the canvas is
// transparent, so the page behind shows. Sharp on high-density screens and at any size it's shown, and white
// like paper even in dark mode, since a drawing's colours are its own. Reports pointer events in
// drawing coordinates, whatever size the canvas is shown at and however it's zoomed. The wheel pans, and
// zooms with Ctrl or ⌘ held (as a trackpad pinch does).
export default function DrawingCanvas({
  commands,
  view = fitted,
  outline,
  handle,
  onPointer,
  onView,
}: {
  commands: DrawCommand[]
  view?: Viewport
  outline?: Bounds | null // the selected shape's box, drawn dashed
  handle?: Point | null // where its resize handle goes
  onPointer?: (phase: PointerPhase, point: Point) => void
  onView?: (view: Viewport) => void
}) {
  const canvas = useRef<HTMLCanvasElement>(null)
  // How wide the canvas is shown, in CSS pixels: drawn at that size, not stretched from a smaller one.
  const [shownWidth, setShownWidth] = useState(drawingWidth)

  useEffect(() => {
    const element = canvas.current
    if (!element || typeof ResizeObserver === 'undefined') return
    const observer = new ResizeObserver(([entry]) => setShownWidth(entry.contentRect.width || drawingWidth))
    observer.observe(element)
    return () => observer.disconnect()
  }, [])

  useEffect(() => {
    const context = canvas.current?.getContext('2d')
    if (!canvas.current || !context) return
    const density = Math.min(maxDensity, (window.devicePixelRatio || 1) * Math.max(1, shownWidth / drawingWidth))
    canvas.current.width = Math.round(drawingWidth * density)
    canvas.current.height = Math.round(drawingHeight * density)
    context.setTransform(density, 0, 0, density, 0, 0)
    context.clearRect(0, 0, drawingWidth, drawingHeight)

    const { scale } = view
    context.setTransform(density * scale, 0, 0, density * scale, -view.x * scale * density, -view.y * scale * density)
    // The sheet, lifted off what's behind it by a soft shadow, seen only when zoomed out.
    context.save()
    context.shadowColor = 'rgba(15, 20, 40, 0.18)'
    context.shadowBlur = 18 * density
    context.shadowOffsetY = 3 * density
    context.fillStyle = '#ffffff'
    context.fillRect(0, 0, drawingWidth, drawingHeight)
    context.restore()
    paint(context, commands)

    if (outline) {
      const pad = outlinePadding / scale
      context.setLineDash([6 / scale, 4 / scale])
      context.lineWidth = 1.5 / scale
      context.strokeStyle = selectionColor
      context.strokeRect(outline.x - pad, outline.y - pad, outline.width + 2 * pad, outline.height + 2 * pad)
      context.setLineDash([])
    }
    if (handle) {
      const size = handleSize / scale
      context.lineWidth = 1.5 / scale
      context.strokeStyle = selectionColor
      context.fillStyle = '#ffffff'
      context.beginPath()
      context.roundRect(handle.x - size / 2, handle.y - size / 2, size, size, 2 / scale)
      context.fill()
      context.stroke()
    }
  }, [commands, view, outline, handle, shownWidth])

  // A position on the canvas, in canvas units before zoom, whatever size it's shown at.
  const onCanvas = (element: HTMLCanvasElement, clientX: number, clientY: number): Point => {
    const box = element.getBoundingClientRect()
    return {
      x: (clientX - box.left) * (box.width ? drawingWidth / box.width : 1),
      y: (clientY - box.top) * (box.height ? drawingHeight / box.height : 1),
    }
  }

  // Native and not passive, so the page itself doesn't scroll or zoom as well.
  useEffect(() => {
    const element = canvas.current
    if (!element || !onView) return
    const changeView = onView
    function onWheel(event: WheelEvent) {
      event.preventDefault()
      const unit = event.deltaMode === 1 ? pixelsPerLine : event.deltaMode === 2 ? pixelsPerPage : 1
      if (event.ctrlKey || event.metaKey) {
        const factor = zoomStep ** (-(event.deltaY * unit) / 100)
        changeView(zoomAround(view, factor, onCanvas(element!, event.clientX, event.clientY)))
      } else {
        changeView(panBy(view, event.deltaX * unit, event.deltaY * unit))
      }
    }
    element.addEventListener('wheel', onWheel, { passive: false })
    return () => element.removeEventListener('wheel', onWheel)
  }, [view, onView])

  const report = (phase: PointerPhase) => (event: PointerEvent<HTMLCanvasElement>) => {
    if (!onPointer) return
    // Keep receiving moves while a drag leaves the canvas.
    if (phase === 'down') event.currentTarget.setPointerCapture?.(event.pointerId)
    onPointer(phase, toDrawing(view, onCanvas(event.currentTarget, event.clientX, event.clientY)))
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
