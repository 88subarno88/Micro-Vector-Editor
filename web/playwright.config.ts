import { defineConfig } from '@playwright/test'

// Browser tests of the whole system: the app, built for production, in Chrome, against a running API
// server, export worker, Postgres and Redis. Start those first (see e2e/README.md); each test makes its own
// users and documents, so they can run against a server that has other data.
export default defineConfig({
  testDir: 'e2e',
  testMatch: '*.e2e.ts',
  timeout: 30_000,
  use: {
    baseURL: 'http://localhost:4173',
    channel: 'chrome', // the installed Google Chrome, as GitHub's runners have: no browser download
    trace: 'retain-on-failure',
  },
  // The production build, served with the same proxy to the API as in development.
  webServer: {
    command: 'npm run build && npx vite preview --port 4173 --strictPort',
    url: 'http://localhost:4173',
    reuseExistingServer: !process.env.CI,
    timeout: 120_000,
  },
  reporter: process.env.CI ? [['list'], ['html', { open: 'never' }]] : 'list',
})
