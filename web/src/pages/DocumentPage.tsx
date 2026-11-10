import { useEffect, useState } from 'react'
import { Link, useParams } from 'react-router'
import { api } from '../api/client'
import { attempt, describeFailure } from '../api/errors'
import { useAuth } from '../auth/auth'
import { useCore } from '../core/useCore'
import Editor from '../editor/Editor'
import type { LoadedDocument } from '../editor/useDrawing'

// One document, open in the editor.
export default function DocumentPage() {
  const { core, error: coreError } = useCore()
  const { user } = useAuth()
  const { id = '' } = useParams()
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
          {coreError ? (
            <p className="error" role="alert">{coreError}</p>
          ) : core ? (
            <Editor core={core} loaded={doc} me={user?.id ?? ''} />
          ) : (
            <p className="loading">Loading the drawing engine…</p>
          )}
        </>
      )}
    </main>
  )
}
