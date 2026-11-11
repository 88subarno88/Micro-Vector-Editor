import { useEffect, useState } from 'react'
import { loadCore, type Core } from './core'

// The core for a component: null until it has loaded, or `error` if it couldn't be.
export function useCore(): { core: Core | null; error: string | null } {
  const [state, setState] = useState<{ core: Core | null; error: string | null }>({ core: null, error: null })
  useEffect(() => {
    let current = true
    loadCore().then(
      (core) => current && setState({ core, error: null }),
      () =>
        current && setState({ core: null, error: "The drawing engine couldn't load. Reload the page to try again." }),
    )
    return () => {
      current = false
    }
  }, [])
  return state
}
