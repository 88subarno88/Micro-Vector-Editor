import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest'
import { api, authFetch, onSessionEnded, setAccessToken } from './client'

const origin = 'http://localhost'

// A fake server: records what it receives, and answers each request with `handler`.
function serve(handler: (path: string, auth: string | null, body: string) => Response | Promise<Response>) {
  const seen: { path: string; auth: string | null; body: string }[] = []
  vi.stubGlobal('fetch', async (input: Request | URL, init?: RequestInit) => {
    const request = input instanceof Request ? input : new Request(input, init)
    const entry = {
      path: new URL(request.url).pathname,
      auth: request.headers.get('Authorization'),
      body: await request.text(),
    }
    seen.push(entry)
    return handler(entry.path, entry.auth, entry.body)
  })
  return seen
}

const json = (status: number, body: unknown) =>
  new Response(JSON.stringify(body), { status, headers: { 'Content-Type': 'application/json' } })
const expired = () => json(401, { error: { code: 'token_expired', message: 'refresh it' } })
const tokens = (token: string) => json(200, { accessToken: token, tokenType: 'Bearer', expiresIn: 900 })

beforeEach(() => setAccessToken(null))
afterEach(() => vi.unstubAllGlobals())

describe('authFetch', () => {
  it('sends the access token', async () => {
    setAccessToken('t1')
    const seen = serve(() => json(200, {}))

    await authFetch(new Request(`${origin}/api/me`))

    expect(seen).toEqual([{ path: '/api/me', auth: 'Bearer t1', body: '' }])
  })

  it('refreshes on a 401 and retries the request with its body and the new token', async () => {
    setAccessToken('old')
    const seen = serve((path, auth) => {
      if (path === '/api/auth/refresh') return tokens('new')
      return auth === 'Bearer new' ? json(201, { id: 'd1' }) : expired()
    })

    const response = await authFetch(new Request(`${origin}/api/docs`, { method: 'POST', body: '{"title":"x"}' }))

    expect(response.status).toBe(201)
    expect(seen.map((s) => [s.path, s.auth, s.body])).toEqual([
      ['/api/docs', 'Bearer old', '{"title":"x"}'],
      ['/api/auth/refresh', null, ''],
      ['/api/docs', 'Bearer new', '{"title":"x"}'],
    ])
  })

  it('gets a token after a reload, when there is none yet', async () => {
    const seen = serve((path, auth) =>
      path === '/api/auth/refresh' ? tokens('fresh') : auth ? json(200, {}) : json(401, {}),
    )

    expect((await authFetch(new Request(`${origin}/api/me`))).status).toBe(200)
    expect(seen.at(-1)?.auth).toBe('Bearer fresh')
  })

  it('shares one refresh between requests that fail at the same time', async () => {
    setAccessToken('old')
    let release!: () => void
    const refreshDone = new Promise<void>((resolve) => (release = resolve))
    const seen = serve(async (path, auth) => {
      if (path !== '/api/auth/refresh') return auth === 'Bearer new' ? json(200, {}) : expired()
      await refreshDone
      return tokens('new')
    })

    const all = Promise.all([1, 2, 3].map(() => authFetch(new Request(`${origin}/api/docs`))))
    await vi.waitFor(() => expect(seen.filter((s) => s.path === '/api/docs')).toHaveLength(3))
    release()

    expect((await all).map((r) => r.status)).toEqual([200, 200, 200])
    expect(seen.filter((s) => s.path === '/api/auth/refresh')).toHaveLength(1)
  })

  it('ends the session when the refresh is refused, and returns the original 401', async () => {
    setAccessToken('old')
    const ended = vi.fn()
    const unsubscribe = onSessionEnded(ended)
    const seen = serve((path) => (path === '/api/auth/refresh' ? json(401, {}) : expired()))

    const response = await authFetch(new Request(`${origin}/api/me`))

    expect(response.status).toBe(401)
    expect(ended).toHaveBeenCalledOnce()
    expect(seen).toHaveLength(2) // no retry
    unsubscribe()

    const after = serve(() => json(200, {}))
    await authFetch(new Request(`${origin}/api/me`))
    expect(after[0].auth).toBeNull() // the old token is gone
  })

  it('ends the session when the server cannot be reached for the refresh', async () => {
    const ended = vi.fn()
    const unsubscribe = onSessionEnded(ended)
    serve((path) => {
      if (path === '/api/auth/refresh') throw new TypeError('network down')
      return json(401, {})
    })

    expect((await authFetch(new Request(`${origin}/api/me`))).status).toBe(401)
    expect(ended).toHaveBeenCalledOnce()
    unsubscribe()
  })

  it('does not retry the auth routes', async () => {
    const seen = serve(() => json(401, { error: { code: 'invalid_credentials', message: 'wrong' } }))

    const response = await authFetch(new Request(`${origin}/api/auth/login`, { method: 'POST', body: '{}' }))

    expect(response.status).toBe(401)
    expect(seen).toHaveLength(1)
  })

  it('retries only once', async () => {
    setAccessToken('old')
    const seen = serve((path) => (path === '/api/auth/refresh' ? tokens('new') : expired()))

    expect((await authFetch(new Request(`${origin}/api/me`))).status).toBe(401)
    expect(seen.map((s) => s.path)).toEqual(['/api/me', '/api/auth/refresh', '/api/me'])
  })
})

describe('api', () => {
  it('is typed from the spec and goes through authFetch', async () => {
    setAccessToken('t1')
    const seen = serve(() =>
      json(201, { id: 'd1', ownerId: 'u1', title: 'Plans', headSeq: 0, createdAt: 'x', updatedAt: 'x' }),
    )

    const { data, error } = await api.POST('/api/docs', { baseUrl: origin, body: { title: 'Plans' } })

    expect(error).toBeUndefined()
    expect(data?.title).toBe('Plans')
    expect(seen[0]).toEqual({ path: '/api/docs', auth: 'Bearer t1', body: '{"title":"Plans"}' })
  })
})
