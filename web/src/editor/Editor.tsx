import { useEffect, useRef, useState, type KeyboardEvent, type ReactNode } from 'react'
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
import type { RestoreRequest } from '../pages/VersionPage'
import {
  CircleIcon,
  CursorIcon,
  DuplicateIcon,
  EyeIcon,
  HexagonIcon,
  LineIcon,
  MinusIcon,
  PenIcon,
  PlusIcon,
  RectIcon,
  RedoIcon,
  RoundedRectIcon,
  SelectIcon,
  TextIcon,
  TrashIcon,
  UndoIcon,
} from '../ui/icons'
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

const toolIcons: Record<Tool, typeof SelectIcon> = {
  select: SelectIcon,
  rect: RectIcon,
  roundedRect: RoundedRectIcon,
  circle: CircleIcon,
  line: LineIcon,
  hexagon: HexagonIcon,
  freehand: PenIcon,
  text: TextIcon,
}

// What to do with each tool, shown in the status bar.
const toolHints: Record<Tool, string> = {
  select: 'Click a shape to select it · drag to move it · drag its handle to resize',
  rect: 'Drag to draw a rectangle',
  roundedRect: 'Drag to draw a rounded rectangle',
  circle: 'Drag to draw a circle',
  line: 'Drag to draw a line',
  hexagon: 'Drag to draw a hexagon',
  freehand: 'Drag to draw freely',
  text: 'Click where the text should go, type, then press Enter',
}

// The keys beyond the tools', for the shortcuts sheet.
const otherShortcuts: [string, string][] = [
  ['Undo', 'Ctrl Z'],
  ['Redo', 'Ctrl ⇧ Z'],
  ['Copy · paste', 'Ctrl C · V'],
  ['Duplicate', 'Ctrl D'],
  ['Delete', 'Del'],
  ['Nudge', 'Arrow keys'],
  ['Zoom', 'Ctrl + · −'],
  ['Pan', 'Scroll'],
]

// What kind of shape the core's JSON for one is, in words.
function shapeName(shape: object | null): string {
  const { kind, cornerRadius } = (shape ?? {}) as { kind?: string; cornerRadius?: number }
  if (kind === 'rect') return cornerRadius ? 'Rounded rectangle' : 'Rectangle'
  const names: Record<string, string> = {
    circle: 'Circle',
    line: 'Line',
    hexagon: 'Hexagon',
    freehand: 'Freehand',
    text: 'Text',
  }
  return names[kind ?? ''] ?? 'Shape'
}

// A coordinate as the inspector shows it: to one decimal place, without a trailing ".0".
const coordinate = (value: number) => String(Math.round(value * 10) / 10)

// The editor, filling the window: a bar across the top with the document's `title` and `actions` and who's
// here; the tools down the left; the drawing in the middle; the selected shape's details on the right; and a
// status bar along the bottom. The Select tool picks a shape to move, resize by its handle, restyle, copy,
// duplicate or delete; the others draw. The view zooms and pans. Every edit shows at
// once and is sent to the server, and others' edits appear as they're made, along with their pointers.
// A viewer gets the drawing, zoom and pan and everyone's pointers, but no way to edit: the server would refuse
// every edit anyway. `me` is the signed-in user's ID, whose own pointer isn't drawn. With `restore`, the drawing is changed back
// to that version as soon as it's live, and `onRestored` is called.
export default function Editor({
  core,
  loaded,
  me,
  sync,
  restore: restoreTo,
  onRestored,
  title,
  actions,
}: {
  core: Core
  loaded: LoadedDocument
  me: string
  sync?: SyncOverrides
  restore?: RestoreRequest
  onRestored?: () => void
  title?: ReactNode
  actions?: ReactNode
}) {
  const {
    commands,
    error,
    rejected,
    present,
    connection,
    canUndo,
    canRedo,
    undo,
    redo,
    restore,
    moveCursor,
    ready,
    apply,
    preview,
    shapeAt,
    boundsOf,
    anchorOf,
    shapeOf,
    translated,
  } = useDrawing(core, loaded, sync)
  const readOnly = loaded.role === 'viewer'
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
  const [restored, setRestored] = useState<number | null>(null) // the version just restored
  const [view, setView] = useState<Viewport>(fitted)
  // The shape last copied, as in an addShape op. Only this editor's; not the system clipboard.
  const clipboard = useRef<object | null>(null)

  const failed = (refused: string | null) => refused && `That edit didn't apply: ${refused}`
  const run = (op: object, group?: object) => setProblem(failed(apply(op, group)))

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
    setProblem(failed(refused))
    if (refused) return
    clipboard.current = shape
    setTool('select')
    setSelected(id)
  }

  // Adds a copy of the selected shape beside it, and selects the copy. The clipboard is left alone.
  function duplicateSelected() {
    if (!selected) return
    const original = shapeOf(selected)
    const shape = original && translated(original, pasteOffset, pasteOffset)
    if (!shape) return setProblem("That shape can't be duplicated.")
    const id = newShapeId()
    const refused = apply({ kind: 'addShape', id, shape })
    setProblem(failed(refused))
    if (!refused) setSelected(id)
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
  function moved(id: string, by: Point, group: object) {
    run(moveOp(id, by), group)
    if (drag.current?.mode !== 'move') return // the drag's last part: it has ended
    setChange({ kind: 'move', by: { x: 0, y: 0 } })
    setShown(null)
  }
  const resizeOp = (id: string, factor: number) => ({ kind: 'resize', id, factor })
  // The handle's size on screen, in drawing units at this zoom.
  const grab = handleSize / view.scale
  const onHandle = (point: Point) =>
    handle !== null && Math.abs(point.x - handle.x) <= grab && Math.abs(point.y - handle.y) <= grab

  // Restores a version once live, so the edits are made against the drawing as it is now, not as it loaded.
  const live = connection.status === 'live'
  const restoring = useRef<RestoreRequest | null>(null) // the request already carried out: never twice
  useEffect(() => {
    if (!restoreTo || !live || restoring.current === restoreTo) return
    restoring.current = restoreTo
    const refused = restore(restoreTo.shapes)
    // oxlint-disable-next-line react/set-state-in-effect -- it reports on edits the effect has just made
    setProblem(refused && `The version couldn't be fully restored: ${refused}`)
    setRestored(refused ? null : restoreTo.seq)
    onRestored?.()
  }, [restoreTo, live, restore, onRestored])

  // Keyboard shortcuts, except while the user is typing somewhere.
  useEffect(() => {
    function onKey(event: globalThis.KeyboardEvent) {
      if (event.target instanceof HTMLInputElement || event.target instanceof HTMLTextAreaElement) return
      const command = event.ctrlKey || event.metaKey
      const key = event.key.toLowerCase()
      const handled = (() => {
        if (readOnly) {
          if (!command) return false
          if (key === '=' || key === '+') zoomBy(zoomStep)
          else if (key === '-') zoomBy(1 / zoomStep)
          else if (key === '0') setView(fitted)
          else return false
          return true
        }
        if (command) {
          if (key === 'z' && !event.shiftKey) setProblem(undo())
          else if (key === 'z' || key === 'y') setProblem(redo())
          else if (key === 'c') copySelected()
          else if (key === 'x') {
            copySelected()
            deleteSelected()
          } else if (key === 'v') paste()
          else if (key === 'd') duplicateSelected()
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
    if (readOnly) return
    if (phase === 'down') {
      setProblem(null)
      if (tool === 'select' && selected && anchor && handle && onHandle(point)) {
        drag.current = { mode: 'resize', id: selected, anchor, handle }
      } else if (tool === 'select') {
        const hit = shapeAt(point)
        setSelected(hit)
        const group = {} // this drag, undone in one step
        const moves = hit && coalescedMoves((by) => moved(hit, by, group), minimumMove)
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
  const placed = (point: Point) => ({
    left: `${(point.x / drawingWidth) * 100}%`,
    top: `${(point.y / drawingHeight) * 100}%`,
  })
  // Everyone else's pointer that's over the part of the drawing in view.
  const pointers = present.flatMap(({ id, name, cursor }) => {
    if (id === me || !cursor) return []
    const at = toScreen(view, cursor)
    const inView = at.x >= 0 && at.y >= 0 && at.x <= drawingWidth && at.y <= drawingHeight
    return inView ? [{ id, name, at }] : []
  })

  // The frame around everything: the top bar, then `body`.
  const studio = (body: ReactNode) => (
    <section className="studio">
      <header className="studio-bar">
        <div className="studio-bar-start">
          {title}
          {readOnly ? (
            <span className="view-only-badge">
              <EyeIcon width={16} height={16} />
              View only
            </span>
          ) : (
            <span className="role-chip">{loaded.role === 'owner' ? 'Owner' : 'Editor'}</span>
          )}
        </div>
        <div className="studio-bar-end">
          <OnlineUsers users={present} me={me} />
          {actions}
        </div>
      </header>
      {body}
    </section>
  )

  if (error)
    return studio(
      <div className="studio-message">
        <p className="error" role="alert">
          {error}
        </p>
      </div>,
    )
  if (!ready) return studio(<p className="loading studio-message">Loading the drawing…</p>)

  const outline =
    bounds &&
    (change?.kind === 'move'
      ? { ...bounds, x: bounds.x + change.by.x, y: bounds.y + change.by.y }
      : change?.kind === 'resize'
        ? scaleBounds(bounds, change.anchor, change.factor)
        : bounds)

  const shortcuts = (
    <section className="inspector-section shortcuts-section">
      <h2 className="inspector-title">Shortcuts</h2>
      <dl className="shortcuts">
        {tools.map(({ tool: option, label, key }) => (
          <div key={option}>
            <dt>{label}</dt>
            <dd>
              <kbd>{key.toUpperCase()}</kbd>
            </dd>
          </div>
        ))}
        {otherShortcuts.map(([what, keys]) => (
          <div key={what}>
            <dt>{what}</dt>
            <dd>
              <kbd>{keys}</kbd>
            </dd>
          </div>
        ))}
      </dl>
    </section>
  )

  return studio(
    <>
      <div className={`studio-body${readOnly ? ' no-rail' : ''}`}>
        {!readOnly && (
          <div className="tool-rail" role="toolbar" aria-label="Tools" aria-orientation="vertical">
            {tools.map(({ tool: option, label, key }) => {
              const ToolIcon = toolIcons[option]
              return (
                <button
                  key={option}
                  className="rail-button"
                  aria-pressed={tool === option}
                  onClick={() => chooseTool(option)}
                >
                  <ToolIcon />
                  <span className="visually-hidden">{label}</span>
                  <span className="tip" aria-hidden="true">
                    {label} <kbd>{key.toUpperCase()}</kbd>
                  </span>
                </button>
              )
            })}
            <span className="rail-divider" aria-hidden="true" />
            <button className="rail-button" onClick={() => setProblem(undo())} disabled={!canUndo}>
              <UndoIcon />
              <span className="visually-hidden">Undo</span>
              <span className="tip" aria-hidden="true">
                Undo <kbd>Ctrl Z</kbd>
              </span>
            </button>
            <button className="rail-button" onClick={() => setProblem(redo())} disabled={!canRedo}>
              <RedoIcon />
              <span className="visually-hidden">Redo</span>
              <span className="tip" aria-hidden="true">
                Redo <kbd>Ctrl ⇧ Z</kbd>
              </span>
            </button>
            <button className="rail-button danger" onClick={deleteSelected} disabled={!outline}>
              <TrashIcon />
              <span className="visually-hidden">Delete shape</span>
              <span className="tip" aria-hidden="true">
                Delete <kbd>Del</kbd>
              </span>
            </button>
          </div>
        )}

        <div className={`workspace ${readOnly ? 'view-only' : `tool-${tool}`}`}>
          <div className="canvas-wrap">
            <DrawingCanvas
              commands={shown ?? commands}
              view={view}
              outline={outline}
              handle={change ? null : handle}
              onPointer={onPointer}
              onView={setView}
            />
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
              <div
                key={`${id}:${index}`}
                // Near the right edge, the name goes on the pointer's left, where there's room for it.
                className={`pointer${at.x > drawingWidth * 0.7 ? ' flip' : ''}`}
                role="img"
                aria-label={`${name}'s pointer`}
                style={{ ...placed(at), color: colorFor(id) }}
              >
                <svg width="16" height="22" viewBox="0 0 14 20" aria-hidden="true">
                  <path d="M1 1 L1 16 L5 12 L8 19 L10 18 L7 11 L13 11 Z" fill="currentColor" stroke="#ffffff" />
                </svg>
                <span style={{ background: colorFor(id) }}>{name}</span>
              </div>
            ))}
          </div>
          {/* Floating over the drawing, so a message never moves it. */}
          <div className="toasts">
            {problem && (
              <p className="error toast" role="alert">
                {problem}
              </p>
            )}
            {rejected && (
              <p className="error toast" role="alert">
                {rejected}
              </p>
            )}
            {restored !== null && (
              <p className="toast">Restored the drawing as it was after edit {restored}. Undo takes it back.</p>
            )}
          </div>
        </div>

        <aside className="inspector" aria-label="Details">
          {readOnly ? (
            <>
              <section className="inspector-section inspector-empty">
                <EyeIcon width={28} height={28} />
                <h2>Read-only access</h2>
                <p className="hint">
                  You can watch others edit, follow their pointers and zoom in, but not change this document.
                </p>
              </section>
              <section className="inspector-section">
                <h2 className="inspector-title">Navigate</h2>
                <dl className="shortcuts">
                  {otherShortcuts.slice(-2).map(([what, keys]) => (
                    <div key={what}>
                      <dt>{what}</dt>
                      <dd>
                        <kbd>{keys}</kbd>
                      </dd>
                    </div>
                  ))}
                </dl>
              </section>
            </>
          ) : drawn && outline ? (
            <>
              <section className="inspector-section">
                <h2 className="inspector-title">{shapeName(selected ? shapeOf(selected) : null)}</h2>
                <dl className="geometry">
                  {(
                    [
                      ['X', outline.x],
                      ['Y', outline.y],
                      ['W', outline.width],
                      ['H', outline.height],
                    ] as const
                  ).map(([name, value]) => (
                    <div key={name}>
                      <dt>{name}</dt>
                      <dd>{coordinate(value)}</dd>
                    </div>
                  ))}
                </dl>
              </section>
              <section className="inspector-section">
                <h2 className="inspector-title">Style</h2>
                <StylePanel
                  key={`${selected}:${JSON.stringify(drawn.style)}`}
                  style={drawn.style}
                  canFill={drawn.kind !== 'polyline' || drawn.closed}
                  onPreview={previewStyle}
                  onCommit={commitStyle}
                  onCancel={() => setShown(null)}
                />
              </section>
              <section className="inspector-section">
                <button type="button" className="wide" onClick={duplicateSelected}>
                  <DuplicateIcon width={18} height={18} />
                  Duplicate
                  <kbd>Ctrl D</kbd>
                </button>
              </section>
            </>
          ) : (
            <>
              <section className="inspector-section inspector-empty">
                <CursorIcon width={28} height={28} />
                <h2>Nothing selected</h2>
                <p className="hint">Pick the Select tool and click a shape to see its size and change its style.</p>
              </section>
              {shortcuts}
            </>
          )}
        </aside>
      </div>

      <footer className="status-bar">
        <ConnectionStatus {...connection} />
        <span className="status-hint">{readOnly ? 'Scroll to pan · Ctrl + scroll to zoom' : toolHints[tool]}</span>
        <span className="status-count">
          {commands.length} {commands.length === 1 ? 'shape' : 'shapes'}
        </span>
        <div className="zoom" role="group" aria-label="Zoom">
          <button onClick={() => zoomBy(1 / zoomStep)} aria-label="Zoom out" title="Zoom out (Ctrl −)">
            <MinusIcon width={16} height={16} />
          </button>
          <button onClick={() => setView(fitted)} aria-label="Reset zoom" title="Reset zoom (Ctrl 0)">
            {Math.round(view.scale * 100)}%
          </button>
          <button onClick={() => zoomBy(zoomStep)} aria-label="Zoom in" title="Zoom in (Ctrl +)">
            <PlusIcon width={16} height={16} />
          </button>
        </div>
      </footer>
    </>,
  )
}
