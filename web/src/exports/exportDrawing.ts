import { api, authFetch } from '../api/client'
import { attempt, describeFailure } from '../api/errors'
import type { components } from '../api/schema'

export type ExportFormat = 'svg' | 'png'
type Job = components['schemas']['ExportJob']

export const pollIntervalMs = 1000
// A worker retries a failed export after 5, 10, 20 and 40 s before giving up, so a job can take over a
// minute and a quarter to fail. Waiting 3 minutes leaves room for a busy queue too.
export const maxPolls = 180

const sleep = (ms: number) => new Promise<void>((resolve) => setTimeout(resolve, ms))

// What the user is told while waiting.
export function describeJob(job: Job): string {
  if (job.status === 'queued' && job.error) return 'That went wrong; trying again shortly…'
  if (job.status === 'queued') return 'Waiting for the export to start…'
  return `Making the ${job.format.toUpperCase()}…`
}

// Exports a document: queues the job, waits for a worker to finish it, and fetches the file. Either the
// file, or what to tell the user. Stops early, with nothing to say, once `signal` is aborted.
export async function exportDrawing(
  documentId: string,
  format: ExportFormat,
  { onProgress, signal, wait = sleep }: {
    onProgress?(message: string): void
    signal?: AbortSignal
    wait?(ms: number): Promise<void>
  } = {},
): Promise<{ file: Blob } | { problem: string | null }> {
  const queued = await attempt(() =>
    api.POST('/api/docs/{id}/exports', { params: { path: { id: documentId } }, body: { format } }),
  )
  if (!queued.data) return { problem: describeFailure(queued.error, queued.response) }
  let job = queued.data

  for (let polls = 0; job.status !== 'done'; polls++) {
    if (job.status === 'failed') return { problem: `The export failed: ${job.error ?? 'no reason was given'}.` }
    if (polls === maxPolls) return { problem: 'The export is taking too long. Try again later.' }
    onProgress?.(describeJob(job))
    await wait(pollIntervalMs)
    if (signal?.aborted) return { problem: null }
    const polled = await attempt(() => api.GET('/api/exports/{id}', { params: { path: { id: job.id } } }))
    if (!polled.data) return { problem: describeFailure(polled.error, polled.response) }
    job = polled.data
  }

  // The file needs the access token too, so it's fetched rather than linked to.
  const response = await authFetch(new Request(new URL(job.resultUrl!, globalThis.location.origin))).catch(() => null)
  if (!response?.ok) return { problem: "The export is ready but couldn't be downloaded. Try again." }
  return { file: await response.blob() }
}

// Hands a file to the browser to save, as `name`.
export function saveFile(file: Blob, name: string) {
  const url = URL.createObjectURL(file)
  const link = document.createElement('a')
  link.href = url
  link.download = name
  link.click()
  setTimeout(() => URL.revokeObjectURL(url), 0)
}
