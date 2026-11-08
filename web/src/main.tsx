import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import { BrowserRouter } from 'react-router'
import App from './App.tsx'
import { AuthProvider } from './auth/AuthProvider.tsx'
import { loadCore } from './core/core'
import './index.css'

// Start fetching the WebAssembly core now, so it's ready by the time a document opens.
void loadCore().catch(() => {}) // useCore reports a failure where it matters

createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <BrowserRouter>
      <AuthProvider>
        <App />
      </AuthProvider>
    </BrowserRouter>
  </StrictMode>,
)
