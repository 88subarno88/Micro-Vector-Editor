import { useCallback, useEffect, useState, type FormEvent } from 'react'
import { Link, useNavigate } from 'react-router'
import { api } from '../api/client'
import { attempt, describeFailure } from '../api/errors'
import type { components } from '../api/schema'
import { useAuth } from '../auth/auth'

type Doc = components['schemas']['Document']

const pageSize = 20
// The most the server takes in one upload.
const maxImportBytes = 1024 * 1024
const dateFormat = new Intl.DateTimeFormat(undefined, { dateStyle: 'medium', timeStyle: 'short' })

// One document in the list: open it, rename it in place, or delete it if it's yours.
function DocumentRow({ doc, isOwner, onRenamed, onDeleted }: {
  doc: Doc
  isOwner: boolean
  onRenamed(doc: Doc): void
  onDeleted(id: string): void
}) {
  const [editing, setEditing] = useState(false)
  const [title, setTitle] = useState(doc.title)
  const [error, setError] = useState<string | null>(null)
  const [busy, setBusy] = useState(false)

  async function rename(event: FormEvent) {
    event.preventDefault()
    setBusy(true)
    const { data, error, response } = await attempt(() =>
      api.PATCH('/api/docs/{id}', { params: { path: { id: doc.id } }, body: { title } }),
    )
    setBusy(false)
    if (!data) return setError(describeFailure(error, response))
    setEditing(false)
    setError(null)
    onRenamed(data)
  }

  async function remove() {
    if (!window.confirm(`Delete "${doc.title}"? This can't be undone.`)) return
    setBusy(true)
    const { error, response } = await attempt(() => api.DELETE('/api/docs/{id}', { params: { path: { id: doc.id } } }))
    setBusy(false)
    if (!response?.ok) return setError(describeFailure(error, response))
    onDeleted(doc.id)
  }

  return (
    <li className="doc">
      {editing ? (
        <form className="inline" onSubmit={rename}>
          <input aria-label="Title" required maxLength={200} value={title} onChange={(e) => setTitle(e.target.value)} autoFocus />
          <button type="submit" disabled={busy}>Save</button>
          <button type="button" onClick={() => { setEditing(false); setTitle(doc.title); setError(null) }}>Cancel</button>
        </form>
      ) : (
        <>
          <Link to={`/docs/${doc.id}`} className="title">{doc.title}</Link>
          <span className="meta">Edited {dateFormat.format(new Date(doc.updatedAt))}</span>
          <span className="actions">
            <button onClick={() => setEditing(true)} disabled={busy}>Rename</button>
            {isOwner && <button onClick={() => void remove()} disabled={busy}>Delete</button>}
          </span>
        </>
      )}
      {error && <p className="error" role="alert">{error}</p>}
    </li>
  )
}

// The signed-in user's documents, newest first, with a form to start a new one or import an SVG file.
export default function DocumentsPage() {
  const { user, logOut } = useAuth()
  const navigate = useNavigate()
  const [docs, setDocs] = useState<Doc[]>([])
  const [nextCursor, setNextCursor] = useState<string | null>(null)
  const [loaded, setLoaded] = useState(false)
  const [error, setError] = useState<string | null>(null)
  const [newTitle, setNewTitle] = useState('')
  const [creating, setCreating] = useState(false)

  const loadPage = useCallback(async (cursor?: string) => {
    const { data, error, response } = await attempt(() =>
      api.GET('/api/docs', { params: { query: { limit: pageSize, cursor } } }),
    )
    setLoaded(true)
    if (!data) return setError(describeFailure(error, response))
    setError(null)
    setDocs((current) => (cursor ? [...current, ...data.documents] : data.documents))
    setNextCursor(data.nextCursor ?? null)
  }, [])

  useEffect(() => {
    // Syncs with the server: loadPage sets state only once the server has answered.
    // oxlint-disable-next-line react/set-state-in-effect
    void loadPage()
  }, [loadPage])

  async function create(event: FormEvent) {
    event.preventDefault()
    setCreating(true)
    const { data, error, response } = await attempt(() => api.POST('/api/docs', { body: { title: newTitle } }))
    setCreating(false)
    if (!data) return setError(describeFailure(error, response))
    setError(null)
    setNewTitle('')
    setDocs((current) => [data, ...current])
  }

  // Makes a document from an SVG file, named after the file, and opens it.
  async function importSvg(file: File) {
    if (file.size > maxImportBytes) return setError('That file is over 1 MB, the most that can be imported.')
    setCreating(true)
    const title = file.name.replace(/\.svg$/i, '').slice(0, 100) || 'Imported drawing'
    const svg = await file.text()
    const { data, error, response } = await attempt(() =>
      api.POST('/api/docs', {
        params: { query: { title } },
        body: svg,
        bodySerializer: (body) => body as string,
        headers: { 'Content-Type': 'image/svg+xml' },
      }),
    )
    setCreating(false)
    if (data) return navigate(`/docs/${data.id}`)
    setError(
      error?.error.code === 'invalid_svg'
        ? `That file couldn't be read as SVG (${error.error.message}).`
        : describeFailure(error, response),
    )
  }

  return (
    <main>
      <header className="top">
        <h1>Your documents</h1>
        <span>
          {user?.email} <button onClick={() => void logOut()}>Log out</button>
        </span>
      </header>

      <form className="inline" onSubmit={create}>
        <input aria-label="New document title" placeholder="Untitled drawing" required maxLength={200}
          value={newTitle} onChange={(e) => setNewTitle(e.target.value)} />
        <button type="submit" disabled={creating}>New document</button>
        <label className={`button${creating ? ' disabled' : ''}`}>
          Import SVG
          <input type="file" accept=".svg,image/svg+xml" className="visually-hidden" disabled={creating}
            onChange={(event) => {
              const file = event.target.files?.[0]
              event.target.value = '' // so choosing the same file again still counts
              if (file) void importSvg(file)
            }} />
        </label>
      </form>
      {error && <p className="error" role="alert">{error}</p>}

      {!loaded ? (
        <p className="loading">Loading…</p>
      ) : docs.length === 0 ? (
        <p className="hint">No documents yet. Start one above.</p>
      ) : (
        <ul className="docs">
          {docs.map((doc) => (
            <DocumentRow key={doc.id} doc={doc} isOwner={doc.ownerId === user?.id}
              onRenamed={(renamed) => setDocs((current) => current.map((d) => (d.id === renamed.id ? renamed : d)))}
              onDeleted={(id) => setDocs((current) => current.filter((d) => d.id !== id))} />
          ))}
        </ul>
      )}
      {nextCursor && <button onClick={() => void loadPage(nextCursor)}>Load more</button>}
    </main>
  )
}
