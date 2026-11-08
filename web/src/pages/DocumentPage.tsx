import { useEffect, useMemo, useState } from 'react'
import { Link, useParams } from 'react-router'
import { api } from '../api/client'
import { attempt, describeFailure } from '../api/errors'
import type { components } from '../api/schema'
import type { Core } from '../core/core'
import { useCore } from '../core/useCore'
import DrawingCanvas from '../editor/DrawingCanvas'
import type { DrawCommand } from '../editor/paint'

type Loaded = components['schemas']['DocumentWithDrawing']

// Rebuilds the drawing in the WebAssembly core from what the server sent, its snapshot then the ops after it,
// and returns how to draw it, or why it couldn't be rebuilt.
function rebuild(core: Core, loaded: Loaded): { commands: DrawCommand[] } | { error: string } {
  const drawing = new core.Document()
  try {
    if (loaded.snapshot) {
      const result = JSON.parse(drawing.loadJson(JSON.stringify(loaded.snapshot.state)))
      if (result.error) return { error: `The drawing couldn't be read: ${result.error.message}` }
    }
    for (const op of loaded.ops) {
      const result = JSON.parse(drawing.applyOp(JSON.stringify(op.op)))
      if (result.error) return { error: `Edit ${op.seq} didn't apply: ${result.error.message}` }
    }
    return { commands: JSON.parse(drawing.drawCommands()) }
  } finally {
    drawing.delete()
  }
}

// One document, drawn. Editing comes next.
export default function DocumentPage() {
  const { core, error: coreError } = useCore()
  const { id = '' } = useParams()
  const [doc, setDoc] = useState<Loaded | null>(null)
  const [error, setError] = useState<string | null>(null)
  const drawing = useMemo(() => (core && doc ? rebuild(core, doc) : null), [core, doc])
  const failure = coreError ?? (drawing && 'error' in drawing ? drawing.error : null)

  useEffect(() => {
    let current = true
    void attempt(() => api.GET('/api/docs/{id}', { params: { path: { id } } })).then(({ data, error, response }) => {
      if (!current) return
      if (data) setDoc(data)
      else setError(describeFailure(error, response))
    })
    return () => {
      current = false
    }
  }, [id])

  return (
    <main>
      <p>
        <Link to="/">← All documents</Link>
      </p>
      {error ? (
        <p className="error" role="alert">{error}</p>
      ) : !doc ? (
        <p className="loading">Loading…</p>
      ) : (
        <>
          <h1>{doc.document.title}</h1>
          <p className="hint">
            You are {doc.role === 'owner' ? 'the owner' : `an ${doc.role}`}.
          </p>
          {failure ? (
            <p className="error" role="alert">{failure}</p>
          ) : drawing && 'commands' in drawing ? (
            <DrawingCanvas commands={drawing.commands} />
          ) : (
            <p className="loading">Loading the drawing engine…</p>
          )}
        </>
      )}
    </main>
  )
}
