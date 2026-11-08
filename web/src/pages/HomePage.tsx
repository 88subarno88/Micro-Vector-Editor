import { useAuth } from '../auth/auth'

// Where a signed-in user lands. The document list replaces the placeholder in 29.4.
export default function HomePage() {
  const { user, logOut } = useAuth()
  return (
    <main>
      <header className="top">
        <h1>Micro-Vector Editor</h1>
        <span>
          {user?.email} <button onClick={() => void logOut()}>Log out</button>
        </span>
      </header>
      <p>Your documents will appear here.</p>
    </main>
  )
}
