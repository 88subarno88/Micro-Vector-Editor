import { useCallback, useEffect, useState, type FormEvent } from 'react'
import { Link, useNavigate } from 'react-router'
import { api } from '../api/client'
import { attempt, describeFailure } from '../api/errors'
import type { components } from '../api/schema'
import { useAuth } from '../auth/auth'
import AppHeader from '../ui/AppHeader'
import DocThumb from '../ui/DocThumb'
import { timeAgo } from '../ui/time'
import { FileIcon, PencilIcon, PlusIcon, TrashIcon, UploadIcon } from '../ui/icons'

type Doc = components['schemas']['Document']

const pageSize = 20
// The most the server takes in one upload.
const maxImportBytes = 1024 * 1024

// One document in the list: open it, rename it in place, or delete it if it's yours.
function DocumentRow({
  doc,
  isOwner,
  onRenamed,
  onDeleted,
}: {
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
    <li className={`doc${editing ? ' editing' : ''}`}>
      <Link to={`/docs/${doc.id}`} className="doc-thumb" aria-hidden="true" tabIndex={-1}>
        <DocThumb id={doc.id} />
      </Link>
      <div className="doc-body">
        {editing ? (
          <form className="doc-rename" onSubmit={rename}>
            <input
              aria-label="Title"
              required
              maxLength={200}
              value={title}
              onChange={(e) => setTitle(e.target.value)}
              autoFocus
            />
            <div className="doc-rename-actions">
              <button type="submit" className="primary" disabled={busy}>
                Save
              </button>
              <button
                type="button"
                className="ghost"
                onClick={() => {
                  setEditing(false)
                  setTitle(doc.title)
                  setError(null)
                }}
              >
                Cancel
              </button>
            </div>
          </form>
        ) : (
          <>
            <Link to={`/docs/${doc.id}`} className="doc-title">
              {doc.title}
            </Link>
            <span className="doc-meta">
              {isOwner ? 'Yours' : 'Shared with you'} · Edited {timeAgo(doc.updatedAt)}
            </span>
            <span className="doc-actions">
              <button
                className="ghost icon"
                onClick={() => setEditing(true)}
                disabled={busy}
                aria-label="Rename"
                title="Rename"
              >
                <PencilIcon />
              </button>
              {isOwner && (
                <button
                  className="ghost icon danger"
                  onClick={() => void remove()}
                  disabled={busy}
                  aria-label="Delete"
                  title="Delete"
                >
                  <TrashIcon />
                </button>
              )}
            </span>
          </>
        )}
        {error && (
          <p className="error" role="alert">
            {error}
          </p>
        )}
      </div>
    </li>
  )
}

// The signed-in user's documents, newest first, with a form to start a new one or import an SVG file.
export default function DocumentsPage() {
  const { user } = useAuth()
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
    <>
      <AppHeader />
      <main className="page">
        <div className="page-head">
          <div>
            <h1>Your documents</h1>
            <p className="hint">Drawings you own, and ones shared with you.</p>
          </div>
        </div>

        <form className="new-doc" onSubmit={create}>
          <input
            aria-label="New document title"
            placeholder="Name a new drawing"
            required
            maxLength={200}
            value={newTitle}
            onChange={(e) => setNewTitle(e.target.value)}
          />
          <button type="submit" className="primary" disabled={creating}>
            <PlusIcon />
            New document
          </button>
          <label className={`button${creating ? ' disabled' : ''}`}>
            <UploadIcon />
            Import SVG
            <input
              type="file"
              accept=".svg,image/svg+xml"
              className="visually-hidden"
              disabled={creating}
              onChange={(event) => {
                const file = event.target.files?.[0]
                event.target.value = '' // so choosing the same file again still counts
                if (file) void importSvg(file)
              }}
            />
          </label>
        </form>
        {error && (
          <p className="error" role="alert">
            {error}
          </p>
        )}

        {!loaded ? (
          <p className="loading">Loading…</p>
        ) : docs.length === 0 ? (
          <div className="empty">
            <FileIcon width={40} height={40} />
            <p>No documents yet. Start one above.</p>
            <p className="hint">Name a drawing, or import an SVG file you already have.</p>
          </div>
        ) : (
          <ul className="docs" aria-label="Documents">
            {docs.map((doc) => (
              <DocumentRow
                key={doc.id}
                doc={doc}
                isOwner={doc.ownerId === user?.id}
                onRenamed={(renamed) => setDocs((current) => current.map((d) => (d.id === renamed.id ? renamed : d)))}
                onDeleted={(id) => setDocs((current) => current.filter((d) => d.id !== id))}
              />
            ))}
          </ul>
        )}
        {nextCursor && (
          <div className="more">
            <button onClick={() => void loadPage(nextCursor)}>Load more</button>
          </div>
        )}
      </main>
    </>
  )
}
