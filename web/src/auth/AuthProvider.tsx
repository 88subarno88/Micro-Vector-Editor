import { useCallback, useEffect, useMemo, useState, type ReactNode } from 'react'
import { api, onSessionEnded, setAccessToken, type ApiError } from '../api/client'
import { attempt, describeFailure } from '../api/errors'
import { AuthContext, type Auth, type User } from './auth'

// How a login answered: with tokens, or not.
type LoginAnswer = { data?: { accessToken: string }; error?: ApiError; response?: Response }

export function AuthProvider({ children }: { children: ReactNode }) {
  const [status, setStatus] = useState<Auth['status']>('checking')
  const [user, setUser] = useState<User | null>(null)

  const signOutLocally = useCallback(() => {
    setAccessToken(null)
    setUser(null)
    setStatus('signedOut')
  }, [])

  // Asks who we are; with no access token yet, the client gets one with the refresh cookie if there is one.
  const loadUser = useCallback(async () => {
    const { data } = await attempt(() => api.GET('/api/me'))
    if (data) {
      setUser(data)
      setStatus('signedIn')
    } else {
      signOutLocally()
    }
  }, [signOutLocally])

  useEffect(() => {
    // Syncs with the server: loadUser sets state only once the server has answered.
    // oxlint-disable-next-line react/set-state-in-effect
    void loadUser()
    return onSessionEnded(signOutLocally)
  }, [loadUser, signOutLocally])

  // Keeps the access token from a login's answer and loads who it belongs to. Null, or why it failed.
  const finishLogin = useCallback(
    async ({ data, error, response }: LoginAnswer) => {
      if (!data) return describeFailure(error, response)
      setAccessToken(data.accessToken)
      await loadUser()
      return null
    },
    [loadUser],
  )

  const logIn = useCallback(
    async (email: string, password: string) =>
      finishLogin(await attempt(() => api.POST('/api/auth/login', { body: { email, password } }))),
    [finishLogin],
  )

  const tryDemo = useCallback(async () => finishLogin(await attempt(() => api.POST('/api/auth/demo'))), [finishLogin])

  const signUp = useCallback(
    async (email: string, password: string) => {
      const { error, response } = await attempt(() => api.POST('/api/auth/signup', { body: { email, password } }))
      if (!response?.ok) return describeFailure(error, response)
      return logIn(email, password)
    },
    [logIn],
  )

  const logOut = useCallback(async () => {
    await attempt(() => api.POST('/api/auth/logout'))
    signOutLocally()
  }, [signOutLocally])

  const value = useMemo(
    () => ({ status, user, logIn, signUp, tryDemo, logOut }),
    [status, user, logIn, signUp, tryDemo, logOut],
  )
  return <AuthContext.Provider value={value}>{children}</AuthContext.Provider>
}
