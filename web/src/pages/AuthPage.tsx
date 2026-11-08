import { useState, type FormEvent } from 'react'
import { Link } from 'react-router'
import { useAuth } from '../auth/auth'

const copy = {
  login: { title: 'Log in', submit: 'Log in', busy: 'Logging in…', other: 'No account yet?', otherLink: 'Sign up', to: '/signup' },
  signup: { title: 'Create an account', submit: 'Sign up', busy: 'Signing up…', other: 'Have an account?', otherLink: 'Log in', to: '/login' },
}

// The login and signup forms, which differ only in wording and what they call. Both end logged in.
export default function AuthPage({ mode }: { mode: 'login' | 'signup' }) {
  const { logIn, signUp } = useAuth()
  const [email, setEmail] = useState('')
  const [password, setPassword] = useState('')
  const [error, setError] = useState<string | null>(null)
  const [busy, setBusy] = useState(false)
  const text = copy[mode]

  async function submit(event: FormEvent) {
    event.preventDefault()
    setBusy(true)
    setError(await (mode === 'login' ? logIn : signUp)(email, password))
    setBusy(false)
  }

  return (
    <main className="auth">
      <h1>{text.title}</h1>
      <form onSubmit={submit}>
        <label>
          Email
          <input type="email" autoComplete="email" required value={email} onChange={(e) => setEmail(e.target.value)} />
        </label>
        <label>
          Password
          <input
            type="password"
            autoComplete={mode === 'login' ? 'current-password' : 'new-password'}
            required
            minLength={mode === 'signup' ? 8 : undefined}
            value={password}
            onChange={(e) => setPassword(e.target.value)}
          />
        </label>
        {mode === 'signup' && <p className="hint">At least 8 characters.</p>}
        {error && (
          <p className="error" role="alert">
            {error}
          </p>
        )}
        <button type="submit" disabled={busy}>
          {busy ? text.busy : text.submit}
        </button>
      </form>
      <p>
        {text.other} <Link to={text.to}>{text.otherLink}</Link>
      </p>
    </main>
  )
}
