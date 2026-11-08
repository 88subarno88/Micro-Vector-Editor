import createClient from 'openapi-fetch'
import type { components, paths } from './schema'

export type ApiError = components['schemas']['Error']

// The access token is kept in memory only, out of reach of anything that reads storage. A reload loses it,
// and the first request then gets a new one with the refresh cookie, which page scripts can't read at all.
let accessToken: string | null = null
let refreshing: Promise<boolean> | null = null
const sessionEndedListeners = new Set<() => void>()

export function setAccessToken(token: string | null) {
  accessToken = token
}

// Called when the session can't be renewed and the user has to log in again. Returns an unsubscribe function.
export function onSessionEnded(listener: () => void): () => void {
  sessionEndedListeners.add(listener)
  return () => sessionEndedListeners.delete(listener)
}

// Gets a new access token with the refresh cookie. Requests failing at the same moment share one refresh:
// each refresh replaces the cookie's token, so a second one sent alongside would be refused.
function refreshAccessToken(origin: string): Promise<boolean> {
  refreshing ??= (async () => {
    try {
      const response = await fetch(new URL('/api/auth/refresh', origin), {
        method: 'POST',
        credentials: 'same-origin',
      })
      if (!response.ok) return false
      const tokens: components['schemas']['Tokens'] = await response.json()
      accessToken = tokens.accessToken
      return true
    } catch {
      return false
    } finally {
      refreshing = null
    }
  })()
  return refreshing
}

function authorize(request: Request): Request {
  if (accessToken) request.headers.set('Authorization', `Bearer ${accessToken}`)
  return request
}

// Sends `request` with the access token. On a 401, renews the token once and retries; if that can't be
// done, ends the session and returns the 401. The auth routes themselves are never retried.
export async function authFetch(request: Request): Promise<Response> {
  const retry = request.clone() // a body can only be sent once
  const response = await fetch(authorize(request))
  const url = new URL(request.url)
  if (response.status !== 401 || url.pathname.startsWith('/api/auth/')) return response

  if (!(await refreshAccessToken(url.origin))) {
    accessToken = null
    sessionEndedListeners.forEach((listener) => listener())
    return response
  }
  return fetch(authorize(retry))
}

// Every API call goes through this: paths, parameters, bodies and responses are typed from the OpenAPI
// spec (regenerate with `npm run gen:api` when it changes). The base URL is the page's own origin, made
// absolute so that the client also works where fetch can't resolve relative URLs, as in tests.
export const api = createClient<paths>({ baseUrl: globalThis.location?.origin, fetch: authFetch })
