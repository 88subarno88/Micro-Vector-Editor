# Web client

React and TypeScript, built with Vite. It runs the C++ core in the browser as WebAssembly and talks to the
C++ server for everything else.

## Development

Needs Node 22 or newer, and the WebAssembly core built with the `wasm` preset (see the main README). Start
the server first, then:

```bash
npm install
npm run wasm       # copy vec.js, vec.wasm and vec.d.ts from ../build/wasm into src/wasm
npm run dev        # http://localhost:5173
```

Run `npm run wasm` again whenever the core changes. The copied files are build outputs, so git ignores them.

The dev server passes `/api` and `/ws` on to the C++ server at `http://localhost:8080`, so the app and the
API share one origin, as they will behind nginx. Set `VEC_API_URL` to use a server elsewhere.

```bash
npm run lint       # oxlint
npm test           # unit tests (Vitest)
npm run build      # type-check, then build into dist/
npm run gen:api    # regenerate src/api/schema.d.ts after changing server/openapi.yaml
```

API calls go through `api` in [src/api/client.ts](src/api/client.ts), typed from the OpenAPI spec. It adds the
access token, which lives only in memory, and on a 401 gets a new one with the refresh cookie and retries once.
