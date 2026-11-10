import { useEffect, useRef, useState, type KeyboardEvent } from 'react'
import type { Core } from '../core/core'
import ConnectionStatus from './ConnectionStatus'
import { colorFor } from './cursors'
import DrawingCanvas, { drawingHeight, drawingWidth, handleSize, type PointerPhase } from './DrawingCanvas'
import { coalescedMoves } from './moves'
import OnlineUsers from './OnlineUsers'
import type { DrawCommand } from './paint'
import { factorFor, handleFor, scaleBounds } from './resize'
import StylePanel, { type StyleChange } from './StylePanel'
import { newShapeId, shapeForDrag, shapeForStroke, textShape, tools, type Point, type Tool } from './tools'
import { useDrawing, type LoadedDocument, type SyncOverrides } from './useDrawing'
import { fitted, toScreen, zoomAround, zoomStep, type Viewport } from './viewport'

// A move this small, in drawing units, is a click on the shape, not a move; likewise a resize this close to 1.
const minimumMove = 0.01
const minimumResize = 0.001
// How far a pasted copy lands from what was copied, as on the desktop; each paste lands this far again.
const pasteOffset = 20
// How far the arrow keys nudge the selected shape, and with Shift held.
const nudge = 1
const bigNudge = 10
const canvasCentre = { x: drawingWidth / 2, y: drawingHeight / 2 }

type Drag =
  | { mode: 'draw'; id: string; points: Point[] } // drawing a new shape
  | { mode: 'move'; id: string; start: Point; moves: ReturnType<typeof coalescedMoves> } // moving the selected shape
  | { mode: 'resize'; id: string; anchor: Point; handle: Point } // resizing it by its handle

// How the selected shape is being changed mid-drag, beyond the edits already made, for drawing its outline
// there.
type Change = { kind: 'move'; by: Point } | { kind: 'resize'; anchor: Point; factor: number }

// The editor: a toolbar above the drawing. The Select tool picks a shape to move, resize by its handle,
// restyle in the style panel, copy or delete; the others draw. The view zooms and pans. Every edit shows at
// once and is sent to the server, and others' edits appear as they're made, along with their pointers.
// `me` is the signed-in user's ID, whose own pointer isn't drawn.
export default function Editor({ core, loaded, me, sync }: {
  core: Core
  loaded: LoadedDocument
  me: string
  sync?: SyncOverrides
}) {
  const {
    commands, error, rejected, present, connection, moveCursor, ready, apply, preview, shapeAt, boundsOf, anchorOf, shapeOf,
    translated,
  } = useDrawing(core, loaded, sync)
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
  const [view, setView] = useState<Viewport>(fitted)
  // The shape last copied, as in an addShape op. Only this editor's; not the system clipboard.
  const clipboard = useRef<object | null>(null)

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

  function copySelected() {
    if (selected) clipboard.current = shapeOf(selected)
  }

  // Adds a copy of the clipboard beside where it was copied from, and selects it. The next paste lands
  // beside this one, so repeated pastes step down and to the right instead of piling up.
  function paste() {
    if (!clipboard.current) return
    const shape = translated(clipboard.current, pasteOffset, pasteOffset)
    if (!shape) return setProblem("The copied shape can't be pasted.")
    const id = newShapeId()
    const refused = apply({ kind: 'addShape', id, shape })
    setProblem(refused)
    if (refused) return
    clipboard.current = shape
    setTool('select')
    setSelected(id)
  }

  const zoomBy = (factor: number) => setView((current) => zoomAround(current, factor, canvasCentre))

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
  // Part of a drag made into an edit while the pointer is still moving: the shape is now drawn there, so
  // nothing is left to preview.
  function moved(id: string, by: Point) {
    run(moveOp(id, by))
    if (drag.current?.mode !== 'move') return // the drag's last part: it has ended
    setChange({ kind: 'move', by: { x: 0, y: 0 } })
    setShown(null)
  }
  const resizeOp = (id: string, factor: number) => ({ kind: 'resize', id, factor })
  // The handle's size on screen, in drawing units at this zoom.
  const grab = handleSize / view.scale
  const onHandle = (point: Point) =>
    handle !== null && Math.abs(point.x - handle.x) <= grab && Math.abs(point.y - handle.y) <= grab

  // Keyboard shortcuts, except while the user is typing somewhere.
  useEffect(() => {
    function onKey(event: globalThis.KeyboardEvent) {
      if (event.target instanceof HTMLInputElement || event.target instanceof HTMLTextAreaElement) return
      const command = event.ctrlKey || event.metaKey
      const key = event.key.toLowerCase()
      const handled = (() => {
        if (command) {
          if (key === 'c') copySelected()
          else if (key === 'x') { copySelected(); deleteSelected() }
          else if (key === 'v') paste()
          else if (key === '=' || key === '+') zoomBy(zoomStep)
          else if (key === '-') zoomBy(1 / zoomStep)
          else if (key === '0') setView(fitted)
          else return false
          return true
        }
        if (event.altKey) return false
        if (key === 'delete' || key === 'backspace') deleteSelected()
        else if (key === 'escape') setSelected(null)
        else if (key.startsWith('arrow') && selected) {
          const step = event.shiftKey ? bigNudge : nudge
          const by = { arrowleft: [-step, 0], arrowright: [step, 0], arrowup: [0, -step], arrowdown: [0, step] }[key]
          if (by) run(moveOp(selected, { x: by[0], y: by[1] }))
        } else {
          const shortcut = tools.find((option) => option.key === key)
          if (!shortcut) return false
          chooseTool(shortcut.tool)
        }
        return true
      })()
      if (handled) event.preventDefault() // so Ctrl+= zooms the drawing, not the page
    }
    window.addEventListener('keydown', onKey)
    return () => window.removeEventListener('keydown', onKey)
  })

  function onPointer(phase: PointerPhase, point: Point) {
    if (phase === 'move' || phase === 'down') moveCursor(point)
    if (phase === 'down') {
      setProblem(null)
      if (tool === 'select' && selected && anchor && handle && onHandle(point)) {
        drag.current = { mode: 'resize', id: selected, anchor, handle }
      } else if (tool === 'select') {
        const hit = shapeAt(point)
        setSelected(hit)
        const moves = hit && coalescedMoves((by) => moved(hit, by), minimumMove)
        drag.current = hit && moves ? { mode: 'move', id: hit, start: point, moves } : null
      } else {
        drag.current = { mode: 'draw', id: newShapeId(), points: [point] }
      }
      return
    }

    const current = drag.current
    if (!current) return
    if (phase === 'move') {
      if (current.mode === 'move') {
        current.moves.to({ x: point.x - current.start.x, y: point.y - current.start.y })
        const by = current.moves.unsent()
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
    if (current.mode === 'move') {
      // A cancelled move stops where it got to: some of it may already be on the server.
      current.moves.end(phase === 'cancel' ? undefined : { x: point.x - current.start.x, y: point.y - current.start.y })
      return
    }
    if (phase === 'cancel') return
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

  // Where the text being typed starts, on the canvas as it's zoomed.
  const textAtOnScreen = textAt && toScreen(view, textAt)
  // As a position over the canvas, which is shown at whatever size the page allows.
  const placed = (point: Point) => ({ left: `${(point.x / drawingWidth) * 100}%`, top: `${(point.y / drawingHeight) * 100}%` })
  // Everyone else's pointer that's over the part of the drawing in view.
  const pointers = present.flatMap(({ id, name, cursor }) => {
    if (id === me || !cursor) return []
    const at = toScreen(view, cursor)
    const inView = at.x >= 0 && at.y >= 0 && at.x <= drawingWidth && at.y <= drawingHeight
    return inView ? [{ id, name, at }] : []
  })

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
      <div className="presence">
        <ConnectionStatus {...connection} />
        <OnlineUsers users={present} me={me} />
      </div>
      <div className="toolbar" role="toolbar" aria-label="Tools">
        {tools.map(({ tool: option, label, key }) => (
          <button key={option} aria-pressed={tool === option} title={`${label} (${key.toUpperCase()})`}
            onClick={() => chooseTool(option)}>
            {label}
          </button>
        ))}
        <button className="danger" onClick={deleteSelected} disabled={!outline} title="Delete shape (Delete)">
          Delete shape
        </button>
        <span className="zoom" role="group" aria-label="Zoom">
          <button onClick={() => zoomBy(1 / zoomStep)} aria-label="Zoom out" title="Zoom out (Ctrl −)">−</button>
          <button onClick={() => setView(fitted)} aria-label="Reset zoom" title="Reset zoom (Ctrl 0)">
            {Math.round(view.scale * 100)}%
          </button>
          <button onClick={() => zoomBy(zoomStep)} aria-label="Zoom in" title="Zoom in (Ctrl +)">+</button>
        </span>
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
        <DrawingCanvas commands={shown ?? commands} view={view} outline={outline} handle={change ? null : handle}
          onPointer={onPointer} onView={setView} />
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
            style={placed(textAtOnScreen!)}
          />
        )}
        {pointers.map(({ id, name, at }, index) => (
          // A user with two tabs open has two pointers, so the index keeps keys apart.
          <div key={`${id}:${index}`} className="pointer" role="img" aria-label={`${name}'s pointer`}
            style={{ ...placed(at), color: colorFor(id) }}>
            <svg width="14" height="20" viewBox="0 0 14 20" aria-hidden="true">
              <path d="M1 1 L1 16 L5 12 L8 19 L10 18 L7 11 L13 11 Z" fill="currentColor" stroke="#ffffff" />
            </svg>
            <span style={{ background: colorFor(id) }}>{name}</span>
          </div>
        ))}
      </div>
      {problem && <p className="error" role="alert">That edit didn't apply: {problem}</p>}
      {rejected && <p className="error" role="alert">{rejected}</p>}
    </section>
  )
}
