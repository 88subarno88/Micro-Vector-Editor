import createVecModule, { type MainModule } from '../wasm/vec.js'

// The C++ core, compiled to WebAssembly: the same code the server runs, so the editor can apply, undo and
// hit-test edits exactly as the server will. Copy it in with `npm run wasm` after building the wasm preset.
export type Core = MainModule

let loading: Promise<Core> | null = null

// Loads the core once; every caller shares the same promise. After a failure the next call tries again.
export function loadCore(): Promise<Core> {
  loading ??= createVecModule().catch((error: unknown) => {
    loading = null
    throw error
  })
  return loading
}
