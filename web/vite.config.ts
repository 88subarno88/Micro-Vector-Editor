import react from '@vitejs/plugin-react'
import { defineConfig } from 'vite'

// In development, the C++ server answers /api and /ws, so the app and the API share one origin, as they
// will behind nginx. Point VEC_API_URL elsewhere to use another server.
const api = process.env.VEC_API_URL ?? 'http://localhost:8080'

export default defineConfig({
  plugins: [react()],
  server: {
    proxy: {
      '/api': api,
      '/ws': { target: api, ws: true },
    },
  },
})
