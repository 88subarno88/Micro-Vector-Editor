// @vitest-environment jsdom
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest'
import { setAccessToken } from '../api/client'
import { exportDrawing, maxPolls } from './exportDrawing'

type Status = 'queued' | 'running' | 'done' | 'failed'
const job = (status: Status, error: string | null = null) => ({
  id: 'j1',
  documentId: 'd1',
  format: 'svg',
  status,
  attempts: 1,
  error,
  createdAt: 'x',
  resultUrl: status === 'done' ? '/api/exports/j1/file' : null,
})
const json = (status: number, body: unknown) =>
  new Response(JSON.stringify(body), { status, headers: { 'Content-Type': 'application/json' } })

// A server whose job goes through `statuses`, one per poll, then stays at the last.
function server(statuses: { status: Status; error?: string }[], file = new Response('<svg/>', { status: 200 })) {
  const requests: string[] = []
  let polls = 0
  vi.stubGlobal('fetch', async (request: Request) => {
    const { pathname } = new URL(request.url)
    requests.push(`${request.method} ${pathname} ${request.headers.get('Authorization')}`)
    if (request.method === 'POST') return json(202, job('queued'))
    if (pathname.endsWith('/file')) return file
    const next = statuses[Math.min(polls++, statuses.length - 1)]
    return json(200, job(next.status, next.error ?? null))
  })
  return requests
}

const noWait = () => Promise.resolve()

beforeEach(() => setAccessToken('valid'))
afterEach(() => vi.unstubAllGlobals())

describe('exportDrawing', () => {
  it('queues the export, waits while the worker makes it, then fetches the file with the token', async () => {
    const requests = server([{ status: 'running' }, { status: 'done' }])
    const progress: string[] = []

    const result = await exportDrawing('d1', 'svg', { onProgress: (m) => progress.push(m), wait: noWait })

    expect('file' in result && (await result.file.text())).toBe('<svg/>')
    expect(progress).toEqual(['Waiting for the export to start…', 'Making the SVG…'])
    expect(requests).toEqual([
      'POST /api/docs/d1/exports Bearer valid',
      'GET /api/exports/j1 Bearer valid',
      'GET /api/exports/j1 Bearer valid',
      'GET /api/exports/j1/file Bearer valid',
    ])
  })

  it('says when an attempt failed and will be tried again, then why the export failed for good', async () => {
    server([
      { status: 'queued', error: 'renderer crashed' },
      { status: 'failed', error: 'renderer crashed' },
    ])
    const progress: string[] = []
    const result = await exportDrawing('d1', 'svg', { onProgress: (m) => progress.push(m), wait: noWait })
    expect(progress).toContain('That went wrong; trying again shortly…')
    expect(result).toEqual({ problem: 'The export failed: renderer crashed.' })
  })

  it('gives up on a job that never finishes', async () => {
    const requests = server([{ status: 'queued' }])
    const result = await exportDrawing('d1', 'svg', { wait: noWait })
    expect(result).toEqual({ problem: 'The export is taking too long. Try again later.' })
    expect(requests.filter((r) => r.startsWith('GET'))).toHaveLength(maxPolls)
  })

  it('stops waiting, with nothing to say, once the page is left', async () => {
    const requests = server([{ status: 'running' }])
    const leaving = new AbortController()
    const result = await exportDrawing('d1', 'svg', { signal: leaving.signal, wait: async () => leaving.abort() })
    expect(result).toEqual({ problem: null })
    expect(requests).toHaveLength(1) // queued, and then not polled
  })

  it("says so when the finished file can't be downloaded", async () => {
    server([{ status: 'done' }], json(409, { error: { code: 'not_ready', message: 'x' } }))
    expect(await exportDrawing('d1', 'png', { wait: noWait })).toEqual({
      problem: "The export is ready but couldn't be downloaded. Try again.",
    })
  })
})
