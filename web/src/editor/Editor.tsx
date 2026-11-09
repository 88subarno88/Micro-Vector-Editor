import { useEffect, useRef, useState, type KeyboardEvent } from 'react'
import type { Core } from '../core/core'
import DrawingCanvas, { drawingHeight, drawingWidth, type PointerPhase } from './DrawingCanvas'
import type { DrawCommand } from './paint'
import { newShapeId, shapeForDrag, shapeForStroke, textShape, tools, type Point, type Tool } from './tools'
import { useDrawing, type LoadedDocument } from './useDrawing'

// A move this small, in drawing units, is a click on the shape, not a move.
const minimumMove = 0.01

type Drag =
  | { mode: 'draw'; id: string; points: Point[] } // drawing a new shape
  | { mode: 'move'; id: string; start: Point } // moving the selected shape

// The editor: a toolbar above the drawing. The Select tool picks a shape to move or delete; the others
// draw. Edits change the drawing in this browser only, until edits are sent to the server (step 31).
export default function Editor({ core, loaded }: { core: Core; loaded: LoadedDocument }) {
  const { commands, error, ready, apply, preview, shapeAt, boundsOf } = useDrawing(core, loaded)
  const [tool, setTool] = useState<Tool>('select')
  const [selected, setSelected] = useState<string | null>(null)
  // The drag in progress. A ref, not state: pointer moves can arrive faster than React renders, and each
  // must see the points before it.
  const drag = useRef<Drag | null>(null)
  const [shown, setShown] = useState<DrawCommand[] | null>(null) // the drawing mid-drag
  const [offset, setOffset] = useState<Point>({ x: 0, y: 0 }) // how far the selected shape is being moved
  const [textAt, setTextAt] = useState<Point | null>(null) // where text is being typed
  const [text, setText] = useState('')
  const [problem, setProblem] = useState<string | null>(null)

  const run = (op: object) => setProblem(apply(op))

  function deleteSelected() {
    if (!selected) return
    run({ kind: 'deleteShape', id: selected })
    setSelected(null)
  }

  // Delete or Backspace removes the selected shape, unless the user is typing somewhere.
  useEffect(() => {
    function onKey(event: globalThis.KeyboardEvent) {
      const typing = event.target instanceof HTMLInputElement || event.target instanceof HTMLTextAreaElement
      if (!typing && (event.key === 'Delete' || event.key === 'Backspace') && selected) {
        event.preventDefault()
        deleteSelected()
      }
    }
    window.addEventListener('keydown', onKey)
    return () => window.removeEventListener('keydown', onKey)
  })

  function chooseTool(next: Tool) {
    setTool(next)
    if (next !== 'select') setSelected(null)
  }

  // The op for a drawing drag so far, if it amounts to a shape.
  function drawOp(id: string, points: Point[]) {
    if (tool === 'freehand') return shapeForStroke(points, id)
    if (tool === 'select' || tool === 'text') return null
    return shapeForDrag(tool, points[0], points[points.length - 1], id)
  }

  const moveOp = (id: string, by: Point) => ({ kind: 'move', id, dx: by.x, dy: by.y })

  function onPointer(phase: PointerPhase, point: Point) {
    if (phase === 'down') {
      setProblem(null)
      if (tool === 'select') {
        const hit = shapeAt(point)
        setSelected(hit)
        drag.current = hit ? { mode: 'move', id: hit, start: point } : null
      } else {
        drag.current = { mode: 'draw', id: newShapeId(), points: [point] }
      }
      return
    }

    const current = drag.current
    if (!current) return
    if (phase === 'move') {
      if (current.mode === 'move') {
        const by = { x: point.x - current.start.x, y: point.y - current.start.y }
        setOffset(by)
        setShown(preview(moveOp(current.id, by)))
      } else {
        current.points.push(point)
        const op = drawOp(current.id, current.points)
        setShown(op ? preview(op) : null)
      }
      return
    }

    drag.current = null
    setShown(null)
    setOffset({ x: 0, y: 0 })
    if (phase === 'cancel') return
    if (current.mode === 'move') {
      const by = { x: point.x - current.start.x, y: point.y - current.start.y }
      if (Math.abs(by.x) > minimumMove || Math.abs(by.y) > minimumMove) run(moveOp(current.id, by))
      return
    }
    current.points.push(point)
    const op = drawOp(current.id, current.points)
    if (op) run(op)
    if (tool === 'text') {
      setText('')
      setTextAt(current.points[0])
    }
  }

  function finishText(keep: boolean) {
    if (!textAt) return
    const op = keep ? textShape(textAt, text, newShapeId()) : null
    setTextAt(null)
    if (op) run(op)
  }

  function onTextKey(event: KeyboardEvent<HTMLInputElement>) {
    if (event.key === 'Enter') finishText(true)
    if (event.key === 'Escape') finishText(false)
  }

  if (error) return <p className="error" role="alert">{error}</p>
  if (!ready) return <p className="loading">Loading the drawing…</p>

  const bounds = selected ? boundsOf(selected) : null
  const outline = bounds && { ...bounds, x: bounds.x + offset.x, y: bounds.y + offset.y }

  return (
    <section className="editor">
      <div className="toolbar" role="toolbar" aria-label="Tools">
        {tools.map(({ tool: option, label }) => (
          <button key={option} aria-pressed={tool === option} onClick={() => chooseTool(option)}>
            {label}
          </button>
        ))}
        <button className="danger" onClick={deleteSelected} disabled={!outline}>
          Delete shape
        </button>
      </div>
      <div className={`canvas-wrap tool-${tool}`}>
        <DrawingCanvas commands={shown ?? commands} outline={outline} onPointer={onPointer} />
        {textAt && (
          <input
            className="text-entry"
            aria-label="Text"
            placeholder="Type, then press Enter"
            autoFocus
            value={text}
            onChange={(e) => setText(e.target.value)}
            onKeyDown={onTextKey}
            onBlur={() => finishText(true)}
            style={{ left: `${(textAt.x / drawingWidth) * 100}%`, top: `${(textAt.y / drawingHeight) * 100}%` }}
          />
        )}
      </div>
      {problem && <p className="error" role="alert">That edit didn't apply: {problem}</p>}
      <p className="hint">Changes aren't saved yet: they stay in this browser tab until live editing arrives.</p>
    </section>
  )
}
