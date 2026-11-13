import { useEffect, useState, type FormEvent } from 'react'
import { Link } from 'react-router'
import { api } from '../api/client'
import { attempt } from '../api/errors'
import { useAuth } from '../auth/auth'
import AuthArt from '../ui/AuthArt'
import { HistoryIcon, LayersIcon, SparkleIcon, UsersIcon } from '../ui/icons'
import Logo from '../ui/Logo'

const copy = {
  login: {
    title: 'Log in',
    lead: 'Welcome back. Pick up where you left off.',
    submit: 'Log in',
    busy: 'Logging in…',
    other: 'No account yet?',
    otherLink: 'Sign up',
    to: '/signup',
  },
  signup: {
    title: 'Create an account',
    lead: 'Free, and ready in a moment.',
    submit: 'Sign up',
    busy: 'Signing up…',
    other: 'Have an account?',
    otherLink: 'Log in',
    to: '/login',
  },
}

const points = [
  { icon: UsersIcon, text: 'Draw together, with everyone’s pointer in view' },
  { icon: LayersIcon, text: 'The same C++ engine in your browser and on the server' },
  { icon: HistoryIcon, text: 'Every edit kept: look back, and restore any version' },
]

// The login and signup forms, which differ only in wording and what they call. Both end logged in. Where
// the server has a demo user, the login page also offers to try it. Beside the form, what the app is.
export default function AuthPage({ mode }: { mode: 'login' | 'signup' }) {
  const { logIn, signUp, tryDemo } = useAuth()
  const [demo, setDemo] = useState(false)
  const [email, setEmail] = useState('')
  const [password, setPassword] = useState('')
  const [error, setError] = useState<string | null>(null)
  const [busy, setBusy] = useState(false)
  const text = copy[mode]

  useEffect(() => {
    if (mode !== 'login') return
    let current = true
    void attempt(() => api.GET('/api/auth/demo')).then(({ data }) => current && setDemo(Boolean(data)))
    return () => {
      current = false
    }
  }, [mode])

  async function demoLogin() {
    setBusy(true)
    setError(await tryDemo())
    setBusy(false)
  }

  async function submit(event: FormEvent) {
    event.preventDefault()
    setBusy(true)
    setError(await (mode === 'login' ? logIn : signUp)(email, password))
    setBusy(false)
  }

  return (
    <div className="auth-page">
      <aside className="auth-brand">
        <Logo />
        <div className="auth-pitch">
          <h2>
            Vector drawing, <span className="gradient-text">together in real time.</span>
          </h2>
          <ul>
            {points.map(({ icon: PointIcon, text: point }) => (
              <li key={point}>
                <PointIcon />
                {point}
              </li>
            ))}
          </ul>
        </div>
        <AuthArt />
      </aside>

      <main className="auth-main">
        <div className="auth-card">
          <span className="auth-card-logo">
            <Logo />
          </span>
          <h1>{text.title}</h1>
          <p className="auth-lead">{text.lead}</p>
          <form onSubmit={submit}>
            <label>
              Email
              <input
                type="email"
                autoComplete="email"
                placeholder="you@example.com"
                required
                value={email}
                onChange={(e) => setEmail(e.target.value)}
              />
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
            <button type="submit" className="primary wide" disabled={busy}>
              {busy ? text.busy : text.submit}
            </button>
          </form>
          {demo && (
            <>
              <div className="divider">
                <span>or</span>
              </div>
              <div className="demo">
                <button type="button" className="wide" onClick={() => void demoLogin()} disabled={busy}>
                  <SparkleIcon />
                  Try the demo
                </button>
                <p className="hint">Just looking? A shared account with sample drawings, no password needed.</p>
              </div>
            </>
          )}
          <p className="auth-switch">
            {text.other} <Link to={text.to}>{text.otherLink}</Link>
          </p>
        </div>
      </main>
    </div>
  )
}
