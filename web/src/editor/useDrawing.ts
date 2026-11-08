import { useCallback, useEffect, useState } from 'react'
import type { components } from '../api/schema'
import type { Core } from '../core/core'
import type { Document as CoreDocument } from '../wasm/vec.js'
import type { DrawCommand } from './paint'

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

  // How the drawing would look with a shape added, without keeping it: for showing a shape while it's drawn.
  const preview = useCallback(
    (add: { id: string }): DrawCommand[] => {
      if (!drawing) return []
      const added: Result = JSON.parse(drawing.applyOp(JSON.stringify(add)))
      if ('error' in added) return commandsOf(drawing)
      const shown = commandsOf(drawing)
      drawing.applyOp(JSON.stringify({ kind: 'deleteShape', id: add.id }))
      return shown
    },
    [drawing],
  )

  return { commands, error, ready: drawing !== null, apply, preview }
}
