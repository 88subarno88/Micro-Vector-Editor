import { useEffect, useState } from 'react'
import { Link, useLocation, useNavigate, useParams } from 'react-router'
import { api } from '../api/client'
import { attempt, describeFailure } from '../api/errors'
import { useAuth } from '../auth/auth'
import { useCore } from '../core/useCore'
import Editor from '../editor/Editor'
import type { LoadedDocument } from '../editor/useDrawing'
import ExportButtons from './ExportButtons'
import type { RestoreRequest } from './VersionPage'

// One document, open in the editor.
export default function DocumentPage() {
  const { core, error: coreError } = useCore()
  const { user } = useAuth()
  const { id = '' } = useParams()
  const navigate = useNavigate()
  // A version to restore, sent from its page.
  const restore = (useLocation().state as { restore?: RestoreRequest } | null)?.restore
  const [doc, setDoc] = useState<LoadedDocument | null>(null)
  const [error, setError] = useState<string | null>(null)

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
      <p className="page-links">
        <Link to="/">← All documents</Link>
        <Link to={`/docs/${id}/history`}>History</Link>
      </p>
      {error ? (
        <p className="error" role="alert">
          {error}
        </p>
      ) : !doc ? (
        <p className="loading">Loading…</p>
      ) : (
        <>
          <h1>{doc.document.title}</h1>
          <p className="hint">You are {doc.role === 'owner' ? 'the owner' : `an ${doc.role}`}.</p>
          <ExportButtons documentId={id} title={doc.document.title} />
          {coreError ? (
            <p className="error" role="alert">
              {coreError}
            </p>
          ) : core ? (
            <Editor
              core={core}
              loaded={doc}
              me={user?.id ?? ''}
              restore={restore}
              // Done once: a reload mustn't restore it again.
              onRestored={() => navigate('.', { replace: true, state: null })}
            />
          ) : (
            <p className="loading">Loading the drawing engine…</p>
          )}
        </>
      )}
    </main>
  )
}
