import { useCallback, useEffect, useState } from 'react'
import type { components } from '../api/schema'
import type { Core } from '../core/core'
import type { Document as CoreDocument } from '../wasm/vec.js'
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

// What applyOp and loadJson return.
type Result = { ok: true } | { error: { code: string; message: string } }

// Rebuilds the drawing from what the server sent, its snapshot then the ops after it. Null, or why not.
function rebuild(drawing: CoreDocument, loaded: LoadedDocument): string | null {
  if (loaded.snapshot) {
    const result: Result = JSON.parse(drawing.loadJson(JSON.stringify(loaded.snapshot.state)))
    if ('error' in result) return `The drawing couldn't be read: ${result.error.message}`
  }
  for (const op of loaded.ops) {
    const result: Result = JSON.parse(drawing.applyOp(JSON.stringify(op.op)))
    if ('error' in result) return `Edit ${op.seq} didn't apply: ${result.error.message}`
  }
  return null
}

const commandsOf = (drawing: CoreDocument): DrawCommand[] => JSON.parse(drawing.drawCommands())

// A document's drawing, held in the WebAssembly core while the page is open.
export function useDrawing(core: Core, loaded: LoadedDocument) {
  const [drawing, setDrawing] = useState<CoreDocument | null>(null)
  const [commands, setCommands] = useState<DrawCommand[]>([])
  const [error, setError] = useState<string | null>(null)

  useEffect(() => {
    const created = new core.Document()
    const failure = rebuild(created, loaded)
    // Creates an object in WebAssembly memory, which must be freed when the page closes, so it can't be
    // made during render.
    // oxlint-disable-next-line react/set-state-in-effect
    setError(failure)
    setCommands(failure ? [] : commandsOf(created))
    setDrawing(failure ? null : created)
    return () => created.delete()
  }, [core, loaded])

  // Applies an edit. Null, or why it was refused.
  const apply = useCallback(
    (op: object): string | null => {
      if (!drawing) return 'The drawing is still loading.'
      const result: Result = JSON.parse(drawing.applyOp(JSON.stringify(op)))
      if ('error' in result) return result.error.message
      setCommands(commandsOf(drawing))
      return null
    },
    [drawing],
  )

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
  const shapeAt = useCallback((point: Point) => (drawing?.hitTest(point.x, point.y) || null), [drawing])

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

  return { commands, error, ready: drawing !== null, apply, preview, shapeAt, boundsOf, anchorOf }
}
