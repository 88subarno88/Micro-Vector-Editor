import type { ReactNode } from 'react'
import { Navigate, Route, Routes, useLocation } from 'react-router'
import { useAuth } from './auth/auth'
import AuthPage from './pages/AuthPage'
import DocumentPage from './pages/DocumentPage'
import DocumentsPage from './pages/DocumentsPage'
import HistoryPage from './pages/HistoryPage'
import VersionPage from './pages/VersionPage'

// Sends a signed-out visitor to the login page, remembering where they were going.
function SignedInOnly({ children }: { children: ReactNode }) {
  const { status } = useAuth()
  const location = useLocation()
  if (status === 'checking') return <p className="loading">Loading…</p>
  if (status === 'signedOut') return <Navigate to="/login" replace state={{ from: location.pathname }} />
  return children
}

// Sends a signed-in visitor on from the login and signup pages, to where they were going.
function SignedOutOnly({ children }: { children: ReactNode }) {
  const { status } = useAuth()
  const from = (useLocation().state as { from?: string } | null)?.from ?? '/'
  if (status === 'checking') return <p className="loading">Loading…</p>
  if (status === 'signedIn') return <Navigate to={from} replace />
  return children
}

export default function App() {
  return (
    <Routes>
      <Route
        path="/login"
        element={
          <SignedOutOnly>
            <AuthPage mode="login" />
          </SignedOutOnly>
        }
      />
      <Route
        path="/signup"
        element={
          <SignedOutOnly>
            <AuthPage mode="signup" />
          </SignedOutOnly>
        }
      />
      <Route
        path="/"
        element={
          <SignedInOnly>
            <DocumentsPage />
          </SignedInOnly>
        }
      />
      <Route
        path="/docs/:id"
        element={
          <SignedInOnly>
            <DocumentPage />
          </SignedInOnly>
        }
      />
      <Route
        path="/docs/:id/history"
        element={
          <SignedInOnly>
            <HistoryPage />
          </SignedInOnly>
        }
      />
      <Route
        path="/docs/:id/history/:seq"
        element={
          <SignedInOnly>
            <VersionPage />
          </SignedInOnly>
        }
      />
      <Route path="*" element={<Navigate to="/" replace />} />
    </Routes>
  )
}
