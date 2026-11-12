import { createContext, useContext } from 'react'
import type { components } from '../api/schema'

export type User = components['schemas']['User']

export interface Auth {
  // 'checking' until the server has said whether a refresh cookie still logs us in.
  status: 'checking' | 'signedIn' | 'signedOut'
  user: User | null
  // Each resolves to a message to show, or null on success.
  logIn(email: string, password: string): Promise<string | null>
  signUp(email: string, password: string): Promise<string | null>
  // Logs in as the shared demo user, which needs no password.
  tryDemo(): Promise<string | null>
  logOut(): Promise<void>
}

export const AuthContext = createContext<Auth | null>(null)

export function useAuth(): Auth {
  const auth = useContext(AuthContext)
  if (!auth) throw new Error('useAuth must be used inside AuthProvider')
  return auth
}
