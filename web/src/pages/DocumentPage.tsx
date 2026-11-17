import { useEffect, useState, type ReactNode } from 'react'
import { Link, useLocation, useNavigate, useParams } from 'react-router'
import { api } from '../api/client'
import { attempt, describeFailure } from '../api/errors'
import { useAuth } from '../auth/auth'
import { useCore } from '../core/useCore'
import Editor from '../editor/Editor'
import type { LoadedDocument } from '../editor/useDrawing'
import { HistoryIcon } from '../ui/icons'
import { LogoMark } from '../ui/Logo'
import ExportButtons from './ExportButtons'
import type { RestoreRequest } from './VersionPage'

// The start of the editor's top bar: home, then the document's name.
function Title({ name }: { name: string | null }) {
  return (
    <>
      <Link to="/" className="studio-home" aria-label="All documents" title="All documents">
        <LogoMark size={28} />
      </Link>
      <Link to="/" className="studio-crumb">
        Documents
      </Link>
      <span className="studio-crumb-sep" aria-hidden="true">
        /
      </span>
      {name === null ? <span className="studio-title skeleton" /> : <h1 className="studio-title">{name}</h1>}
    </>
  )
}

// The editor's frame while there's no editor yet: loading, or why the document can't be shown.
function Placeholder({ children }: { children: ReactNode }) {
  return (
    <section className="studio">
      <header className="studio-bar">
        <div className="studio-bar-start">
          <Title name={null} />
        </div>
      </header>
      <div className="studio-message">{children}</div>
    </section>
  )
}

// One document, open in the editor, filling the window.
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

  useEffect(() => {
    if (doc) document.title = `${doc.document.title} — Micro-Vector Editor`
    return () => {
      document.title = 'Micro-Vector Editor'
    }
  }, [doc])

  const problem = error ?? coreError
  let body: ReactNode
  if (problem) {
    body = (
      <Placeholder>
        <p className="error" role="alert">
          {problem}
        </p>
        <Link to="/" className="button">
          Back to your documents
        </Link>
      </Placeholder>
    )
  } else if (!doc || !core) {
    body = (
      <Placeholder>
        <p className="loading">{doc ? 'Loading the drawing engine…' : 'Loading…'}</p>
      </Placeholder>
    )
  } else {
    body = (
      <Editor
        core={core}
        loaded={doc}
        me={user?.id ?? ''}
        restore={restore}
        // Done once: a reload mustn't restore it again.
        onRestored={() => navigate('.', { replace: true, state: null })}
        title={<Title name={doc.document.title} />}
        actions={
          <>
            <Link to={`/docs/${id}/history`} className="button ghost" title="Every edit, and earlier versions">
              <HistoryIcon width={18} height={18} />
              <span className="label">History</span>
            </Link>
            <ExportButtons documentId={id} title={doc.document.title} />
          </>
        }
      />
    )
  }
  return <main className="studio-page">{body}</main>
}
