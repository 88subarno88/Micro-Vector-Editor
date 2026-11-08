import { useCallback, useEffect, useMemo, useState, type ReactNode } from 'react'
import { api, onSessionEnded, setAccessToken } from '../api/client'
import { AuthContext, describeFailure, type Auth, type User } from './auth'

// Runs `request`, turning a network failure into a missing response rather than a throw.
async function attempt<T>(
  request: () => Promise<T>,
): Promise<T | { data: undefined; error: undefined; response: undefined }> {
  try {
    return await request()
  } catch {
    return { data: undefined, error: undefined, response: undefined }
  }
}

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

  const logIn = useCallback(
    async (email: string, password: string) => {
      const { data, error, response } = await attempt(() => api.POST('/api/auth/login', { body: { email, password } }))
      if (!data) return describeFailure(error, response)
      setAccessToken(data.accessToken)
      await loadUser()
      return null
    },
    [loadUser],
  )

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

  const value = useMemo(() => ({ status, user, logIn, signUp, logOut }), [status, user, logIn, signUp, logOut])
  return <AuthContext.Provider value={value}>{children}</AuthContext.Provider>
}
