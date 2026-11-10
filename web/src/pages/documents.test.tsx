// @vitest-environment jsdom
import '@testing-library/jest-dom/vitest'
import { cleanup, render, screen, within } from '@testing-library/react'
import userEvent from '@testing-library/user-event'
import { MemoryRouter } from 'react-router'
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest'
import { setAccessToken } from '../api/client'
import App from '../App'
import { AuthProvider } from '../auth/AuthProvider'
import { FakeSocket } from '../sync/testing'

// A stand-in for the WebAssembly core: every op it's given adds a shape, except one of kind "broken".
vi.mock('../core/core', () => ({
  loadCore: async () => ({
    Document: class {
      shapes = 0
      loadJson() { return '{"ok":true}' }
      applyOp(op: string) {
        if (JSON.parse(op).kind === 'broken') return '{"error":{"code":"shape_not_found","message":"no such shape"}}'
        this.shapes += 1
        return '{"ok":true}'
      }
      drawCommands() { return JSON.stringify(Array(this.shapes).fill({ kind: 'rect' })) }
      shapesJson() { return '{"shapes":[]}' }
      delete() {}
    },
  }),
}))

const me = { id: 'u1', email: 'ada@example.com', createdAt: 'x' }

interface Doc {
  id: string
  ownerId: string
  title: string
  headSeq: number
  createdAt: string
  updatedAt: string
}

const json = (status: number, body: unknown) =>
  new Response(JSON.stringify(body), { status, headers: { 'Content-Type': 'application/json' } })
const failure = (status: number, code: string) => json(status, { error: { code, message: `server says ${code}` } })

// A fake server holding `docs`, newest first, logged in as ada unless `loggedIn` is false. Ada owns the
// documents whose ownerId is u1; on the others she's a viewer, so the server refuses her renames.
function server(docs: Doc[], { loggedIn = true } = {}) {
  const state = {
    docs: [...docs], loggedIn, nextId: 100, ops: [addOp('s1'), addOp('s2')] as unknown[],
    history: [] as { seq: number }[], // newest first
  }
  vi.stubGlobal('fetch', async (input: Request | URL, init?: RequestInit) => {
    const request = input instanceof Request ? input : new Request(input, init)
    const url = new URL(request.url)
    const route = `${request.method} ${url.pathname.replace(/^\/api\/docs\/[^/]+(\/ops)?$/, '/api/docs/:id$1')}`
    const id = url.pathname.split('/')[3]
    const doc = state.docs.find((d) => d.id === id)
    const authorized = request.headers.get('Authorization') === 'Bearer valid'

    if (url.pathname === '/api/auth/refresh') {
      return state.loggedIn ? json(200, { accessToken: 'valid', tokenType: 'Bearer', expiresIn: 900 }) : failure(401, 'x')
    }
    if (url.pathname === '/api/auth/login') {
      state.loggedIn = true
      return json(200, { accessToken: 'valid', tokenType: 'Bearer', expiresIn: 900 })
    }
    if (!authorized) return failure(401, 'unauthorized')

    switch (route) {
      case 'GET /api/me':
        return json(200, me)
      case 'GET /api/docs': {
        const limit = Number(url.searchParams.get('limit'))
        const start = Number(url.searchParams.get('cursor') ?? 0)
        const page = state.docs.slice(start, start + limit)
        const more = start + limit < state.docs.length
        return json(200, { documents: page, nextCursor: more ? String(start + limit) : null })
      }
      case 'POST /api/docs': {
        const { title } = await request.json()
        const created = { id: `d${state.nextId++}`, ownerId: me.id, title, headSeq: 0, createdAt: 'x', updatedAt: '2026-11-09T10:00:00Z' }
        state.docs.unshift(created)
        return json(201, created)
      }
      case 'PATCH /api/docs/:id': {
        if (!doc) return failure(404, 'not_found')
        if (doc.ownerId !== me.id) return failure(403, 'forbidden')
        doc.title = (await request.json()).title
        return json(200, doc)
      }
      case 'DELETE /api/docs/:id':
        state.docs = state.docs.filter((d) => d !== doc)
        return new Response(null, { status: 204 })
      case 'GET /api/docs/:id/ops': {
        if (!doc) return failure(404, 'not_found')
        // Two to a page, so a test can page through a short history.
        const before = Number(url.searchParams.get('before') ?? Infinity)
        const page = state.history.filter((entry) => entry.seq < before).slice(0, 2)
        const more = page.length === 2 && page[1].seq > 1
        return json(200, { ops: page, nextBefore: more ? page[1].seq : null })
      }
      case 'GET /api/docs/:id':
        if (!doc) return failure(404, 'not_found')
        return json(200, { document: doc, role: doc.ownerId === me.id ? 'owner' : 'viewer', snapshot: null, ops: state.ops, opsComplete: true })
    }
    return failure(404, 'not_found')
  })
  return state
}

const addOp = (id: string) => ({ seq: 1, userId: me.id, clientOpId: id, createdAt: 'x', op: { kind: 'addShape', id } })

const doc = (id: string, title: string, ownerId = me.id): Doc => ({
  id, ownerId, title, headSeq: 3, createdAt: 'x', updatedAt: '2026-11-09T09:00:00Z',
})

function renderAt(path: string) {
  render(
    <MemoryRouter initialEntries={[path]}>
      <AuthProvider>
        <App />
      </AuthProvider>
    </MemoryRouter>,
  )
}

const row = async (title: string) => (await screen.findByRole('link', { name: title })).closest('li') as HTMLElement

beforeEach(() => {
  setAccessToken(null)
  // jsdom has no canvas; the drawing component copes without one.
  vi.spyOn(HTMLCanvasElement.prototype, 'getContext').mockReturnValue(null)
  // The editor's live connection, which never opens here.
  vi.stubGlobal('WebSocket', FakeSocket)
})
afterEach(() => {
  cleanup()
  vi.unstubAllGlobals()
  vi.restoreAllMocks()
})

describe('the documents page', () => {
  it('lists your documents, and loads more when there are', async () => {
    server(Array.from({ length: 25 }, (_, i) => doc(`d${i}`, `Doc ${i}`)))
    renderAt('/')

    expect(await screen.findByRole('link', { name: 'Doc 0' })).toBeInTheDocument()
    expect(screen.getAllByRole('listitem')).toHaveLength(20)
    await userEvent.click(screen.getByRole('button', { name: 'Load more' }))

    expect(await screen.findByRole('link', { name: 'Doc 24' })).toBeInTheDocument()
    expect(screen.getAllByRole('listitem')).toHaveLength(25)
    expect(screen.queryByRole('button', { name: 'Load more' })).not.toBeInTheDocument()
  })

  it('says when there are none', async () => {
    server([])
    renderAt('/')
    expect(await screen.findByText(/No documents yet/)).toBeInTheDocument()
  })

  it('creates a document at the top of the list', async () => {
    const state = server([doc('d1', 'Older')])
    renderAt('/')
    await userEvent.type(await screen.findByLabelText('New document title'), 'Plans')
    await userEvent.click(screen.getByRole('button', { name: 'New document' }))

    expect(await screen.findByRole('link', { name: 'Plans' })).toBeInTheDocument()
    expect(screen.getAllByRole('link').map((l) => l.textContent)).toEqual(['Plans', 'Older'])
    expect(state.docs[0].title).toBe('Plans')
  })

  it('renames a document in place', async () => {
    server([doc('d1', 'Draft')])
    renderAt('/')
    await userEvent.click(within(await row('Draft')).getByRole('button', { name: 'Rename' }))
    const input = screen.getByLabelText('Title')
    await userEvent.clear(input)
    await userEvent.type(input, 'Final')
    await userEvent.click(screen.getByRole('button', { name: 'Save' }))

    expect(await screen.findByRole('link', { name: 'Final' })).toBeInTheDocument()
  })

  it("explains when you can't rename a document shared with you", async () => {
    server([doc('d1', 'Theirs', 'u2')])
    renderAt('/')
    await userEvent.click(within(await row('Theirs')).getByRole('button', { name: 'Rename' }))
    await userEvent.type(screen.getByLabelText('Title'), '!')
    await userEvent.click(screen.getByRole('button', { name: 'Save' }))

    expect(await screen.findByRole('alert')).toHaveTextContent("You don't have permission to do that here.")
  })

  it('offers delete only on your own documents, and asks first', async () => {
    const state = server([doc('d1', 'Mine'), doc('d2', 'Theirs', 'u2')])
    renderAt('/')
    expect(within(await row('Theirs')).queryByRole('button', { name: 'Delete' })).not.toBeInTheDocument()

    const confirm = vi.spyOn(window, 'confirm').mockReturnValueOnce(false).mockReturnValueOnce(true)
    const deleteButton = within(await row('Mine')).getByRole('button', { name: 'Delete' })
    await userEvent.click(deleteButton) // cancelled
    expect(state.docs).toHaveLength(2)
    await userEvent.click(deleteButton)

    expect(confirm).toHaveBeenCalledTimes(2)
    await vi.waitFor(() => expect(screen.queryByRole('link', { name: 'Mine' })).not.toBeInTheDocument())
    expect(state.docs.map((d) => d.id)).toEqual(['d2'])
  })

  it('opens a document', async () => {
    server([doc('d1', 'Plans')])
    renderAt('/')
    await userEvent.click(await screen.findByRole('link', { name: 'Plans' }))

    expect(await screen.findByRole('heading', { name: 'Plans' })).toBeInTheDocument()
    expect(screen.getByText('You are the owner.')).toBeInTheDocument()
    expect(await screen.findByRole('img', { name: 'The drawing' })).toBeInTheDocument()
  })

  it("says so when the server's edits don't rebuild the drawing", async () => {
    const state = server([doc('d1', 'Plans')])
    state.ops = [addOp('s1'), { ...addOp('s2'), seq: 2, op: { kind: 'broken' } }]
    renderAt('/docs/d1')
    expect(await screen.findByRole('alert')).toHaveTextContent("Edit 2 didn't apply: no such shape")
  })

  it("says so when a document can't be opened", async () => {
    server([])
    renderAt('/docs/missing')
    expect(await screen.findByRole('alert')).toHaveTextContent("That document doesn't exist")
  })

  it('returns to the document you were opening after you log in', async () => {
    server([doc('d1', 'Plans')], { loggedIn: false })
    renderAt('/docs/d1')
    await userEvent.type(await screen.findByLabelText('Email'), 'ada@example.com')
    await userEvent.type(screen.getByLabelText('Password'), 'correct horse')
    await userEvent.click(screen.getByRole('button', { name: 'Log in' }))

    expect(await screen.findByRole('heading', { name: 'Plans' })).toBeInTheDocument()
  })

  describe('the history', () => {
    const entry = (seq: number, userId: string, userEmail: string, op: object) =>
      ({ seq, userId, userEmail, op, createdAt: '2026-11-10T09:30:00Z' })

    it('lists every edit newest first, saying what, who and when, and loads older ones', async () => {
      const state = server([doc('d1', 'Plans')])
      state.history = [
        entry(3, 'u2', 'bob@example.com', { kind: 'move', id: 'a', dx: 5, dy: 0 }),
        entry(2, me.id, me.email, { kind: 'setStyle', id: 'a', fill: 'none' }),
        entry(1, me.id, me.email, { kind: 'addShape', id: 'a', shape: { kind: 'circle' } }),
      ]
      renderAt('/docs/d1')
      await userEvent.click(await screen.findByRole('link', { name: 'History' }))

      const rows = async () => (await screen.findAllByRole('row')).slice(1).map((r) => within(r).getAllByRole('cell').map((c) => c.textContent))
      expect((await rows()).map((cells) => cells.slice(0, 3))).toEqual([
        ['3', 'Moved a shape by 5, 0', 'bob@example.com'],
        ['2', "Changed a shape's style: no fill", 'You'],
      ])
      expect(screen.getAllByRole('row')[1].querySelector('time')).toHaveAttribute('dateTime', '2026-11-10T09:30:00Z')

      await userEvent.click(screen.getByRole('button', { name: 'Load older edits' }))

      expect((await rows()).map((cells) => cells[0])).toEqual(['3', '2', '1'])
      expect(screen.queryByRole('button', { name: 'Load older edits' })).not.toBeInTheDocument()
      expect(screen.getByRole('link', { name: '← Back to the drawing' })).toHaveAttribute('href', '/docs/d1')
    })

    it('says when there are no edits yet', async () => {
      server([doc('d1', 'Plans')])
      renderAt('/docs/d1/history')
      expect(await screen.findByText('No edits yet.')).toBeInTheDocument()
    })

    it("says so when the document can't be seen", async () => {
      server([])
      renderAt('/docs/missing/history')
      expect(await screen.findByRole('alert')).toHaveTextContent("That document doesn't exist")
    })
  })
})
