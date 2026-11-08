import { useEffect, useMemo, useState } from 'react'
import { Link, useParams } from 'react-router'
import { api } from '../api/client'
import { attempt, describeFailure } from '../api/errors'
import type { components } from '../api/schema'
import type { Core } from '../core/core'
import { useCore } from '../core/useCore'

type Loaded = components['schemas']['DocumentWithDrawing']

// Rebuilds the drawing in the WebAssembly core from what the server sent: its snapshot, then the ops after it.
// The number of shapes, or why it couldn't be built.
function countShapes(core: Core, loaded: Loaded): string {
  const drawing = new core.Document()
  try {
    if (loaded.snapshot) {
      const result = JSON.parse(drawing.loadJson(JSON.stringify(loaded.snapshot.state)))
      if (result.error) return `The drawing couldn't be read: ${result.error.message}`
    }
    for (const op of loaded.ops) {
      const result = JSON.parse(drawing.applyOp(JSON.stringify(op.op)))
      if (result.error) return `Edit ${op.seq} didn't apply: ${result.error.message}`
    }
    const shapes = JSON.parse(drawing.shapesJson()).shapes.length
    return `${shapes} ${shapes === 1 ? 'shape' : 'shapes'}, rebuilt in the browser.`
  } finally {
    drawing.delete()
  }
}

// One document. Shows what it is for now; the editor takes over this page in step 30.
export default function DocumentPage() {
  const { core, error: coreError } = useCore()
  const { id = '' } = useParams()
  const [doc, setDoc] = useState<Loaded | null>(null)
  const [error, setError] = useState<string | null>(null)
  const drawing = useMemo(() => (core && doc ? countShapes(core, doc) : null), [core, doc])

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
            You are {doc.role === 'owner' ? 'the owner' : `an ${doc.role}`}. {doc.document.headSeq} edits so far. The
            editor comes next.
          </p>
          <p className={coreError ? 'error' : 'hint'}>{coreError ?? drawing ?? 'Loading the drawing engine…'}</p>
        </>
      )}
    </main>
  )
}
