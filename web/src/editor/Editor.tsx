import { useEffect, useRef, useState, type KeyboardEvent } from 'react'
import type { Core } from '../core/core'
import DrawingCanvas, { drawingHeight, drawingWidth, handleSize, type PointerPhase } from './DrawingCanvas'
import type { DrawCommand } from './paint'
import { factorFor, handleFor, scaleBounds } from './resize'
import StylePanel, { type StyleChange } from './StylePanel'
import { newShapeId, shapeForDrag, shapeForStroke, textShape, tools, type Point, type Tool } from './tools'
import { useDrawing, type LoadedDocument } from './useDrawing'

// A move this small, in drawing units, is a click on the shape, not a move; likewise a resize this close to 1.
const minimumMove = 0.01
const minimumResize = 0.001

type Drag =
  | { mode: 'draw'; id: string; points: Point[] } // drawing a new shape
  | { mode: 'move'; id: string; start: Point } // moving the selected shape
  | { mode: 'resize'; id: string; anchor: Point; handle: Point } // resizing it by its handle

// How the selected shape is being changed mid-drag, for drawing its outline there.
type Change = { kind: 'move'; by: Point } | { kind: 'resize'; anchor: Point; factor: number }

// The editor: a toolbar above the drawing. The Select tool picks a shape to move, resize by its handle,
// restyle in the style panel, or delete; the others draw. Edits change the drawing in this browser only, until edits are sent to the server (step 31).
export default function Editor({ core, loaded }: { core: Core; loaded: LoadedDocument }) {
  const { commands, error, ready, apply, preview, shapeAt, boundsOf, anchorOf } = useDrawing(core, loaded)
  const [tool, setTool] = useState<Tool>('select')
  const [selected, setSelected] = useState<string | null>(null)
  // The drag in progress. A ref, not state: pointer moves can arrive faster than React renders, and each
  // must see the points before it.
  const drag = useRef<Drag | null>(null)
  const [shown, setShown] = useState<DrawCommand[] | null>(null) // the drawing mid-drag
  const [change, setChange] = useState<Change | null>(null)
  const [textAt, setTextAt] = useState<Point | null>(null) // where text is being typed
  const [text, setText] = useState('')
  const [problem, setProblem] = useState<string | null>(null)

  const run = (op: object) => setProblem(apply(op))

  // Where the selected shape is, what it scales around, and so where its handle goes. Read from the core
  // on every render, since any edit can move or reshape it; each is a few small calls.
  const bounds = selected ? boundsOf(selected) : null
  const anchor = selected ? anchorOf(selected) : null
  const handle = bounds && anchor ? handleFor(bounds, anchor) : null
  // The selected shape's own draw command: its committed style, and whether it can be filled.
  const drawn = selected ? commands.find((command) => command.id === selected) : undefined

  const styleOp = (change: StyleChange) => ({ kind: 'setStyle', id: selected, ...change })
  const previewStyle = (change: StyleChange) => setShown(preview(styleOp(change)))
  const commitStyle = (change: StyleChange) => {
    setShown(null)
    run(styleOp(change))
  }

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
  const resizeOp = (id: string, factor: number) => ({ kind: 'resize', id, factor })
  const onHandle = (point: Point) =>
    handle !== null && Math.abs(point.x - handle.x) <= handleSize && Math.abs(point.y - handle.y) <= handleSize

  function onPointer(phase: PointerPhase, point: Point) {
    if (phase === 'down') {
      setProblem(null)
      if (tool === 'select' && selected && anchor && handle && onHandle(point)) {
        drag.current = { mode: 'resize', id: selected, anchor, handle }
      } else if (tool === 'select') {
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
        setChange({ kind: 'move', by })
        setShown(preview(moveOp(current.id, by)))
      } else if (current.mode === 'resize') {
        const factor = factorFor(current.anchor, current.handle, point)
        setChange({ kind: 'resize', anchor: current.anchor, factor })
        setShown(preview(resizeOp(current.id, factor)))
      } else {
        current.points.push(point)
        const op = drawOp(current.id, current.points)
        setShown(op ? preview(op) : null)
      }
      return
    }

    drag.current = null
    setShown(null)
    setChange(null)
    if (phase === 'cancel') return
    if (current.mode === 'move') {
      const by = { x: point.x - current.start.x, y: point.y - current.start.y }
      if (Math.abs(by.x) > minimumMove || Math.abs(by.y) > minimumMove) run(moveOp(current.id, by))
      return
    }
    if (current.mode === 'resize') {
      const factor = factorFor(current.anchor, current.handle, point)
      if (Math.abs(factor - 1) > minimumResize) run(resizeOp(current.id, factor))
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

  const outline =
    bounds &&
    (change?.kind === 'move'
      ? { ...bounds, x: bounds.x + change.by.x, y: bounds.y + change.by.y }
      : change?.kind === 'resize'
        ? scaleBounds(bounds, change.anchor, change.factor)
        : bounds)

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
      {/* Always takes the same space, so selecting a shape never moves the drawing under the pointer. */}
      {drawn ? (
        <StylePanel
          key={`${selected}:${JSON.stringify(drawn.style)}`}
          style={drawn.style}
          canFill={drawn.kind !== 'polyline' || drawn.closed}
          onPreview={previewStyle}
          onCommit={commitStyle}
          onCancel={() => setShown(null)}
        />
      ) : (
        <p className="style-panel hint">Select a shape to change its style.</p>
      )}
      <div className={`canvas-wrap tool-${tool}`}>
        <DrawingCanvas commands={shown ?? commands} outline={outline} handle={change ? null : handle} onPointer={onPointer} />
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
