import type { ReactNode } from 'react'
import { Link } from 'react-router'
import { useAuth } from '../auth/auth'
import { colorFor } from '../editor/cursors'
import { LogOutIcon } from './icons'
import Logo from './Logo'

// The bar across the top of the signed-in pages: the logo, home; anything the page adds; then who's signed
// in, and a way out.
export default function AppHeader({ children }: { children?: ReactNode }) {
  const { user, logOut } = useAuth()
  return (
    <header className="app-header">
      <Link to="/" className="app-header-home" aria-label="All documents">
        <Logo size={28} />
      </Link>
      <div className="app-header-middle">{children}</div>
      {user && (
        <div className="app-header-user">
          <span className="avatar small" style={{ background: colorFor(user.id) }} aria-hidden="true">
            {user.email.charAt(0).toUpperCase()}
          </span>
          <span className="app-header-email">{user.email}</span>
          <button type="button" className="ghost" onClick={() => void logOut()}>
            <LogOutIcon />
            Log out
          </button>
        </div>
      )}
    </header>
  )
}
