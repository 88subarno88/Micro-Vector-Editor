import { useEffect, useState } from 'react'
import { Link, useParams } from 'react-router'
import { api } from '../api/client'
import type { components } from '../api/schema'
import { attempt, describeFailure } from '../api/errors'
import { useAuth } from '../auth/auth'
import { describeEdit } from '../history/describeEdit'

type Entry = components['schemas']['HistoryEntry']

const fetchPage = (id: string, before?: number) =>
  attempt(() => api.GET('/api/docs/{id}/ops', { params: { path: { id }, query: { before } } }))

const when = new Intl.DateTimeFormat(undefined, { dateStyle: 'medium', timeStyle: 'medium' })

// Every edit made to a document, newest first: what it was, who made it, and when.
export default function HistoryPage() {
  const { id = '' } = useParams()
  const { user } = useAuth()
  const [entries, setEntries] = useState<Entry[] | null>(null)
  const [nextBefore, setNextBefore] = useState<number | null>(null)
  const [loading, setLoading] = useState(false)
  const [error, setError] = useState<string | null>(null)

  useEffect(() => {
    let current = true
    void fetchPage(id).then(({ data, error, response }) => {
      if (!current) return
      if (!data) return setError(describeFailure(error, response))
      setEntries(data.ops)
      setNextBefore(data.nextBefore ?? null)
    })
    return () => {
      current = false
    }
  }, [id])

  async function loadOlder() {
    if (nextBefore === null) return
    setLoading(true)
    const { data, error, response } = await fetchPage(id, nextBefore)
    setLoading(false)
    if (!data) return setError(describeFailure(error, response))
    setError(null)
    setEntries((shown) => [...(shown ?? []), ...data.ops])
    setNextBefore(data.nextBefore ?? null)
  }

  return (
    <main>
      <p>
        <Link to={`/docs/${id}`}>← Back to the drawing</Link>
      </p>
      <h1>History</h1>
      {error && (
        <p className="error" role="alert">
          {error}
        </p>
      )}
      {!entries ? (
        !error && <p className="loading">Loading…</p>
      ) : entries.length === 0 ? (
        <p className="hint">No edits yet.</p>
      ) : (
        <table className="history">
          <thead>
            <tr>
              <th scope="col">Edit</th>
              <th scope="col">What</th>
              <th scope="col">Who</th>
              <th scope="col">When</th>
              <th scope="col">
                <span className="visually-hidden">Version</span>
              </th>
            </tr>
          </thead>
          <tbody>
            {entries.map((entry) => (
              <tr key={entry.seq}>
                <td>{entry.seq}</td>
                <td>{describeEdit(entry.op)}</td>
                <td>{entry.userId === user?.id ? 'You' : entry.userEmail}</td>
                <td>
                  <time dateTime={entry.createdAt}>{when.format(new Date(entry.createdAt))}</time>
                </td>
                <td>
                  <Link to={`/docs/${id}/history/${entry.seq}`} aria-label={`View the drawing after edit ${entry.seq}`}>
                    View
                  </Link>
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      )}
      {nextBefore !== null && (
        <button onClick={() => void loadOlder()} disabled={loading}>
          {loading ? 'Loading…' : 'Load older edits'}
        </button>
      )}
    </main>
  )
}
