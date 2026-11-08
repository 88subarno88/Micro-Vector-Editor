// @vitest-environment jsdom
import '@testing-library/jest-dom/vitest'
import { cleanup, render, screen } from '@testing-library/react'
import userEvent from '@testing-library/user-event'
import { MemoryRouter } from 'react-router'
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest'
import { setAccessToken } from '../api/client'
import App from '../App'
import { AuthProvider } from './AuthProvider'
import { describeFailure } from './auth'

const json = (status: number, body: unknown, headers: Record<string, string> = {}) =>
  new Response(JSON.stringify(body), { status, headers: { 'Content-Type': 'application/json', ...headers } })
const failure = (status: number, code: string) => json(status, { error: { code, message: `server says ${code}` } })

// A fake server with one account, ada@example.com / "correct horse". `loggedIn` stands for the refresh
// cookie: whether the browser holds a valid one.
function server({ loggedIn = false, rateLimited = false } = {}) {
  const state = { loggedIn, accounts: new Set(['ada@example.com']), paths: [] as string[] }
  vi.stubGlobal('fetch', async (input: Request | URL, init?: RequestInit) => {
    const request = input instanceof Request ? input : new Request(input, init)
    const path = new URL(request.url).pathname
    state.paths.push(path)
    const body = request.method === 'POST' ? await request.text() : ''
    const { email, password } = body ? JSON.parse(body) : ({} as Record<string, string>)
    switch (path) {
      case '/api/me':
        return request.headers.get('Authorization') === 'Bearer valid'
          ? json(200, { id: 'u1', email: 'ada@example.com', createdAt: 'x' })
          : failure(401, 'unauthorized')
      case '/api/auth/refresh':
        return state.loggedIn ? json(200, { accessToken: 'valid', tokenType: 'Bearer', expiresIn: 900 }) : failure(401, 'invalid_refresh_token')
      case '/api/auth/login':
        if (rateLimited) return json(429, { error: { code: 'rate_limited', message: 'slow down' } }, { 'Retry-After': '42' })
        if (!state.accounts.has(email) || password !== 'correct horse') return failure(401, 'invalid_credentials')
        state.loggedIn = true
        return json(200, { accessToken: 'valid', tokenType: 'Bearer', expiresIn: 900 })
      case '/api/auth/signup':
        if (state.accounts.has(email)) return failure(409, 'email_taken')
        state.accounts.add(email)
        return json(201, { id: 'u2', email, createdAt: 'x' })
      case '/api/auth/logout':
        state.loggedIn = false
        return new Response(null, { status: 204 })
    }
    return failure(404, 'not_found')
  })
  return state
}

function renderAt(path: string) {
  render(
    <MemoryRouter initialEntries={[path]}>
      <AuthProvider>
        <App />
      </AuthProvider>
    </MemoryRouter>,
  )
}

// Waits for the form, which appears once the app has checked whether the visitor is logged in.
async function fillIn(email: string, password: string) {
  await userEvent.type(await screen.findByLabelText('Email'), email)
  await userEvent.type(screen.getByLabelText('Password'), password)
}

beforeEach(() => setAccessToken(null))
afterEach(() => {
  cleanup()
  vi.unstubAllGlobals()
})

describe('the auth pages', () => {
  it('send a signed-out visitor to the login page', async () => {
    server()
    renderAt('/')
    expect(await screen.findByRole('heading', { name: 'Log in' })).toBeInTheDocument()
  })

  it('keep a returning user logged in through the refresh cookie', async () => {
    const state = server({ loggedIn: true })
    renderAt('/')
    expect(await screen.findByText('ada@example.com')).toBeInTheDocument()
    expect(state.paths).toContain('/api/auth/refresh')
  })

  it('show why a login failed', async () => {
    server()
    renderAt('/login')
    await fillIn('ada@example.com', 'wrong password')
    await userEvent.click(await screen.findByRole('button', { name: 'Log in' }))
    expect(await screen.findByRole('alert')).toHaveTextContent('Wrong email or password.')
  })

  it('log in, then log out', async () => {
    const state = server()
    renderAt('/login')
    await fillIn('ada@example.com', 'correct horse')
    await userEvent.click(await screen.findByRole('button', { name: 'Log in' }))

    expect(await screen.findByText('ada@example.com')).toBeInTheDocument()
    await userEvent.click(screen.getByRole('button', { name: 'Log out' }))

    expect(await screen.findByRole('heading', { name: 'Log in' })).toBeInTheDocument()
    expect(state.loggedIn).toBe(false)
  })

  it('sign up and land logged in', async () => {
    server()
    renderAt('/signup')
    await fillIn('new@example.com', 'correct horse')
    await userEvent.click(await screen.findByRole('button', { name: 'Sign up' }))
    expect(await screen.findByText('ada@example.com')).toBeInTheDocument() // the fake server's /api/me
  })

  it('explain a taken email on signup', async () => {
    server()
    renderAt('/signup')
    await fillIn('ada@example.com', 'correct horse')
    await userEvent.click(await screen.findByRole('button', { name: 'Sign up' }))
    expect(await screen.findByRole('alert')).toHaveTextContent('An account with that email already exists.')
  })

  it('say how long to wait when rate limited', async () => {
    server({ rateLimited: true })
    renderAt('/login')
    await fillIn('ada@example.com', 'correct horse')
    await userEvent.click(await screen.findByRole('button', { name: 'Log in' }))
    expect(await screen.findByRole('alert')).toHaveTextContent('Too many attempts. Try again in 42 seconds.')
  })
})

describe('describeFailure', () => {
  it('says how long to wait when rate limited', () => {
    const response = new Response(null, { status: 429, headers: { 'Retry-After': '42' } })
    expect(describeFailure({ error: { code: 'rate_limited', message: 'x' } }, response)).toBe(
      'Too many attempts. Try again in 42 seconds.',
    )
  })

  it('reports a server it cannot reach', () => {
    expect(describeFailure(undefined, undefined)).toMatch(/Can't reach the server/)
  })

  it("falls back to the server's own message for codes it doesn't know", () => {
    expect(describeFailure({ error: { code: 'odd', message: 'An odd thing.' } }, new Response(null, { status: 400 }))).toBe(
      'An odd thing.',
    )
  })
})
