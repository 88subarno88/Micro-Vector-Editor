import { useEffect, useMemo, useState } from 'react'
import { Link, useNavigate, useParams } from 'react-router'
import { api } from '../api/client'
import type { components } from '../api/schema'
import { attempt, describeFailure } from '../api/errors'
import { useCore } from '../core/useCore'
import DrawingCanvas from '../editor/DrawingCanvas'
import type { DrawCommand } from '../editor/paint'
import { rebuild } from '../editor/useDrawing'
import type { ShapeEntry } from '../history/restore'

type Version = components['schemas']['DocumentVersion']

// What a restore hands the editor, through the router's state.
export interface RestoreRequest {
  seq: number
  shapes: ShapeEntry[]
}

// A document as it was after one edit, replayed in a core of its own, with a way to go back to it.
export default function VersionPage() {
  const { id = '', seq = '' } = useParams()
  const navigate = useNavigate()
  const { core, error: coreError } = useCore()
  const [version, setVersion] = useState<Version | null>(null)
  const [error, setError] = useState<string | null>(null)

  useEffect(() => {
    let current = true
    void attempt(() => api.GET('/api/docs/{id}/versions/{seq}', { params: { path: { id, seq: Number(seq) } } })).then(
      ({ data, error, response }) => {
        if (!current) return
        if (data) setVersion(data)
        else setError(describeFailure(error, response))
      },
    )
    return () => {
      current = false
    }
  }, [id, seq])

  // The drawing, replayed in a core document that's freed straight after: only what it says is kept.
  const drawn = useMemo((): { commands: DrawCommand[]; shapes: ShapeEntry[] } | string | null => {
    if (!core || !version) return null
    if (!version.opsComplete) return 'This version is too far back from a saved copy of the drawing to show.'
    const document = new core.Document()
    try {
      return rebuild(document, version) ?? {
        commands: JSON.parse(document.drawCommands()),
        shapes: JSON.parse(document.shapesJson()).shapes,
      }
    } finally {
      document.delete()
    }
  }, [core, version])

  const problem = error ?? coreError ?? (typeof drawn === 'string' ? drawn : null)
  const picture = typeof drawn === 'string' ? null : drawn
  const canRestore = version?.role === 'owner' || version?.role === 'editor'

  function restore() {
    if (!picture) return
    const request: RestoreRequest = { seq: Number(seq), shapes: picture.shapes }
    navigate(`/docs/${id}`, { state: { restore: request } })
  }

  return (
    <main>
      <p>
        <Link to={`/docs/${id}/history`}>← Back to the history</Link>
      </p>
      <h1>As it was after edit {seq}</h1>
      {problem ? (
        <p className="error" role="alert">{problem}</p>
      ) : !picture ? (
        <p className="loading">Loading…</p>
      ) : (
        <>
          {canRestore && (
            <p className="restore">
              <button onClick={restore}>Restore this version</button>
              <span className="hint">The drawing is changed back with ordinary edits: everyone sees them, and you can undo them.</span>
            </p>
          )}
          <div className="canvas-wrap">
            <DrawingCanvas commands={picture.commands} />
          </div>
        </>
      )}
    </main>
  )
}
