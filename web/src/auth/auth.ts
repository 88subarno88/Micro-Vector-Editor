import { createContext, useContext } from 'react'
import type { ApiError } from '../api/client'
import type { components } from '../api/schema'

export type User = components['schemas']['User']

export interface Auth {
  // 'checking' until the server has said whether a refresh cookie still logs us in.
  status: 'checking' | 'signedIn' | 'signedOut'
  user: User | null
  // Each resolves to a message to show, or null on success.
  logIn(email: string, password: string): Promise<string | null>
  signUp(email: string, password: string): Promise<string | null>
  logOut(): Promise<void>
}

export const AuthContext = createContext<Auth | null>(null)

const messages: Record<string, string> = {
  invalid_credentials: 'Wrong email or password.',
  invalid_email: 'Enter a valid email address.',
  invalid_password: 'Use a password of 8 to 1024 characters.',
  email_taken: 'An account with that email already exists. Log in instead.',
}

// What to tell the user about a failed auth request.
export function describeFailure(error: ApiError | undefined, response: Response | undefined): string {
  if (!response) return "Can't reach the server. Check your connection and try again."
  if (response.status === 429) {
    const seconds = Number(response.headers.get('Retry-After'))
    return seconds > 0 ? `Too many attempts. Try again in ${seconds} seconds.` : 'Too many attempts. Try again soon.'
  }
  return (error && messages[error.error.code]) ?? error?.error.message ?? 'Something went wrong. Try again.'
}

export function useAuth(): Auth {
  const auth = useContext(AuthContext)
  if (!auth) throw new Error('useAuth must be used inside AuthProvider')
  return auth
}
