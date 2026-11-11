import { useCallback, useEffect, useState } from 'react'
import { getAccessToken, renewAccessToken } from '../api/client'
import type { components } from '../api/schema'
import type { Core } from '../core/core'
import { restoreOps, type ShapeEntry } from '../history/restore'
import { DocumentSync, type DocumentSyncOptions } from '../sync/DocumentSync'
import type { PresenceUser } from '../sync/messages'
import type { SyncStatus } from '../sync/SyncClient'
import { describeRejection } from '../sync/rejections'
import type { Drawings } from '../sync/SyncEngine'
import type { Document as CoreDocument } from '../wasm/vec.js'
import type { Connection } from './connection'
import { latestEvery } from './cursors'
import { History, type Step } from './history'
import type { DrawCommand } from './paint'
import { anchorFrom } from './resize'
import type { Point } from './tools'

export interface Bounds {
  x: number
  y: number
  width: number
  height: number
}

export type LoadedDocument = components['schemas']['DocumentWithDrawing']

// Replaces how the drawing connects to the server, as tests do.
export type SyncOverrides = Partial<Omit<DocumentSyncOptions, 'drawings' | 'lastSeq' | 'events'>>

// What applyOp and loadJson return.
type Result = { ok: true } | { error: { code: string; message: string } }

const errorIn = (result: string): string | null => {
  const parsed: Result = JSON.parse(result)
  return 'error' in parsed ? parsed.error.message : null
}

// Rebuilds the drawing from what the server sent, its snapshot then the ops after it. Null, or why not.
export function rebuild(drawing: CoreDocument, loaded: Pick<LoadedDocument, 'snapshot' | 'ops'>): string | null {
  if (loaded.snapshot) {
    const failure = errorIn(drawing.loadJson(JSON.stringify(loaded.snapshot.state)))
    if (failure) return `The drawing couldn't be read: ${failure}`
  }
  for (const op of loaded.ops) {
    const failure = errorIn(drawing.applyOp(JSON.stringify(op.op)))
    if (failure) return `Edit ${op.seq} didn't apply: ${failure}`
  }
  return null
}

const commandsOf = (drawing: CoreDocument): DrawCommand[] => JSON.parse(drawing.drawCommands())

const stopped = {
  not_found: 'This document has been deleted, or you no longer have access to it.',
  signed_out: 'You have been signed out. Sign in again to keep editing.',
}

// A document's drawing, held in the WebAssembly core while the page is open and kept in step with the
// server's. Two copies are kept (see SyncEngine): the one the user sees, which every other function here
// reads, and the server's as far as this client knows it.
export function useDrawing(core: Core, loaded: LoadedDocument, overrides?: SyncOverrides) {
  const [drawing, setDrawing] = useState<CoreDocument | null>(null)
  const [sync, setSync] = useState<DocumentSync | null>(null)
  const [cursor, setCursor] = useState<ReturnType<typeof latestEvery> | null>(null) // this user's pointer
  const [present, setPresent] = useState<PresenceUser[]>([]) // everyone looking, this user included
  const [status, setStatus] = useState<SyncStatus>('closed')
  const [joinedOnce, setJoinedOnce] = useState(false)
  const [online, setOnline] = useState(() => navigator.onLine)
  const [pending, setPending] = useState(0) // edits the server hasn't confirmed
  const [commands, setCommands] = useState<DrawCommand[]>([])
  const [error, setError] = useState<string | null>(null)
  const [rejected, setRejected] = useState<string | null>(null) // why the server refused the latest edit
  const [history, setHistory] = useState<History | null>(null) // this user's undo and redo
  const [undoable, setUndoable] = useState({ canUndo: false, canRedo: false })
  const [syncOverrides] = useState(overrides) // read once, when the drawing loads

  useEffect(() => {
    // Objects in WebAssembly memory, which must be freed when the page closes, so they can't be made during
    // render.
    const local = new core.Document()
    const confirmed = new core.Document()
    const failure = rebuild(confirmed, loaded) ?? errorIn(local.loadJson(confirmed.shapesJson()))
    // oxlint-disable-next-line react/set-state-in-effect
    setError(failure)
    setCommands(failure ? [] : commandsOf(local))
    setDrawing(failure ? null : local)
    if (failure) return () => [local, confirmed].forEach((document) => document.delete())

    const steps = new History()
    const showHistory = () => setUndoable({ canUndo: steps.canUndo, canRedo: steps.canRedo })
    showHistory()
    const drawings: Drawings = {
      applyConfirmed: (op) => errorIn(confirmed.applyOp(JSON.stringify(op))),
      applyLocal: (op) => errorIn(local.applyOp(JSON.stringify(op))),
      // A copy through JSON, whose numbers round-trip exactly.
      resetLocal: () => void local.loadJson(confirmed.shapesJson()),
      loadConfirmed: (snapshot) => errorIn(confirmed.loadJson(JSON.stringify(snapshot))),
    }
    const created = new DocumentSync({
      documentId: loaded.document.id,
      drawings,
      lastSeq: loaded.ops.at(-1)?.seq ?? loaded.snapshot?.seq ?? 0,
      getToken: getAccessToken,
      renewToken: renewAccessToken,
      events: {
        onStatus: (next) => {
          setStatus(next)
          if (next === 'live') setJoinedOnce(true)
        },
        onChange: () => {
          setCommands(commandsOf(local))
          setPending(created.engine.pending.length)
        },
        onRejected: (op, reason, message) => {
          setRejected(describeRejection(reason, message))
          steps.forget(op)
          showHistory()
        },
        onPresence: setPresent,
        onDiverged: () => setError('This drawing got out of step with the server. Reload the page to carry on.'),
        onStopped: (reason) => setError(stopped[reason]),
      },
      ...syncOverrides,
    })
    const pointer = latestEvery((point) => created.cursor(point.x, point.y))
    setSync(created)
    setCursor(pointer)
    setHistory(steps)
    created.start()
    return () => {
      pointer.stop()
      created.stop()
      local.delete()
      confirmed.delete()
    }
  }, [core, loaded, syncOverrides])

  // Makes an edit and sends it, having first worked out the op that would reverse it (null if none can).
  const send = useCallback(
    (op: object): { refused: string | null; reverse: object | null } => {
      if (!drawing || !sync) return { refused: 'The drawing is still loading.', reverse: null }
      setRejected(null)
      const reverse = JSON.parse(drawing.invertOp(JSON.stringify(op))).op ?? null
      const refused = sync.edit(op)
      if (!refused) setCommands(commandsOf(drawing))
      setPending(sync.engine.pending.length)
      return { refused, reverse }
    },
    [drawing, sync],
  )

  const showHistory = useCallback(
    () => history && setUndoable({ canUndo: history.canUndo, canRedo: history.canRedo }),
    [history],
  )

  // Makes an edit, which can be undone. Moves from one drag share a `group`, and are undone together. Null,
  // or why the drawing refused it.
  const apply = useCallback(
    (op: object, group?: object): string | null => {
      const { refused, reverse } = send(op)
      if (!refused && reverse) history?.record(op, reverse, group)
      showHistory()
      return refused
    },
    [send, history, showHistory],
  )

  // Reverses this user's latest step, or redoes the latest one undone, by sending new edits. Null, or why it
  // couldn't be: the step is then dropped, and any of its edits already sent stay.
  const travel = useCallback(
    (direction: 'undo' | 'redo'): string | null => {
      const step = direction === 'undo' ? history?.takeUndo() : history?.takeRedo()
      if (!history || !step) return null
      const back: Step = { made: [], reverse: [] }
      for (const op of step.reverse) {
        const { refused, reverse } = send(op)
        if (refused) {
          showHistory()
          return `That can't be ${direction === 'undo' ? 'undone' : 'redone'} any more: someone else has changed it.`
        }
        back.made.push(op)
        if (reverse) back.reverse.unshift(reverse)
      }
      if (direction === 'undo') history.pushRedo(back)
      else history.pushUndo(back)
      showHistory()
      return null
    },
    [send, history, showHistory],
  )
  // Makes the drawing `target` again, with edits that are undone in one step. Null, or why an edit couldn't
  // be made: those before it stay.
  const restore = useCallback(
    (target: ShapeEntry[]): string | null => {
      if (!drawing) return 'The drawing is still loading.'
      const group = {}
      for (const op of restoreOps(JSON.parse(drawing.shapesJson()).shapes, target)) {
        const refused = apply(op, group)
        if (refused) return refused
      }
      return null
    },
    [drawing, apply],
  )

  const undo = useCallback(() => travel('undo'), [travel])
  const redo = useCallback(() => travel('redo'), [travel])

  // Follows the browser's network. Back online, there's no point waiting out a reconnect delay.
  useEffect(() => {
    const changed = () => {
      setOnline(navigator.onLine)
      if (navigator.onLine) sync?.reconnectNow()
    }
    window.addEventListener('online', changed)
    window.addEventListener('offline', changed)
    return () => {
      window.removeEventListener('online', changed)
      window.removeEventListener('offline', changed)
    }
  }, [sync])

  const connection: Connection = { status, joinedOnce, online, pending }

  // Tells the others where this user's pointer is, in drawing units.
  const moveCursor = useCallback((point: Point) => cursor?.to(point), [cursor])

  // How the drawing would look after `op`, without keeping it: applies it, looks, then applies its inverse.
  // For showing a shape while it's drawn or dragged. Unchanged if the op doesn't apply.
  const preview = useCallback(
    (op: object): DrawCommand[] => {
      if (!drawing) return []
      const text = JSON.stringify(op)
      const inverse = JSON.parse(drawing.invertOp(text))
      if (!inverse.op || 'error' in JSON.parse(drawing.applyOp(text))) return commandsOf(drawing)
      const shown = commandsOf(drawing)
      drawing.applyOp(JSON.stringify(inverse.op))
      return shown
    },
    [drawing],
  )

  // The topmost shape at a point, or null.
  const shapeAt = useCallback((point: Point) => drawing?.hitTest(point.x, point.y) || null, [drawing])

  // The box around a shape, or null if it's gone.
  const boundsOf = useCallback(
    (id: string): Bounds | null => (drawing ? JSON.parse(drawing.boundsOf(id)) : null),
    [drawing],
  )

  // The point a shape scales around, measured by scaling it by 2 and back. 2 and its inverse, 0.5, are
  // exact in floating point, so the shape ends up exactly as it was. Null if it can't be resized.
  const anchorOf = useCallback(
    (id: string): Point | null => {
      if (!drawing) return null
      const before: Bounds | null = JSON.parse(drawing.boundsOf(id))
      const probe = JSON.stringify({ kind: 'resize', id, factor: 2 })
      const inverse = JSON.parse(drawing.invertOp(probe))
      if (!before || !inverse.op || 'error' in JSON.parse(drawing.applyOp(probe))) return null
      const after: Bounds = JSON.parse(drawing.boundsOf(id))
      drawing.applyOp(JSON.stringify(inverse.op))
      return anchorFrom(before, after, 2)
    },
    [drawing],
  )

  // A shape's data, as in an addShape op, for copying it. Null if it's gone.
  const shapeOf = useCallback(
    (id: string): object | null => {
      if (!drawing) return null
      const shapes: { id: string; shape: object }[] = JSON.parse(drawing.shapesJson()).shapes
      return shapes.find((entry) => entry.id === id)?.shape ?? null
    },
    [drawing],
  )

  // `shape` moved by (dx, dy), by the core, which knows every kind of shape's geometry. Null if it can't be.
  const translated = useCallback(
    (shape: object, dx: number, dy: number): object | null => {
      if (!drawing) return null
      const result = JSON.parse(drawing.translatedShape(JSON.stringify(shape), dx, dy))
      return result.shape ?? null
    },
    [drawing],
  )

  return {
    commands,
    error,
    rejected,
    present,
    connection,
    ...undoable,
    undo,
    redo,
    restore,
    moveCursor,
    ready: drawing !== null,
    apply,
    preview,
    shapeAt,
    boundsOf,
    anchorOf,
    shapeOf,
    translated,
  }
}
