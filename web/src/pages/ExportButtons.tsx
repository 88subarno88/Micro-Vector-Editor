import { useEffect, useRef, useState } from 'react'
import { exportDrawing, saveFile, type ExportFormat } from '../exports/exportDrawing'

// A file name from the document's title: the characters file systems refuse are replaced.
const fileName = (title: string, format: ExportFormat) => `${title.replace(/[\\/:*?"<>|]/g, '_').trim() || 'drawing'}.${format}`

// Download the document as SVG or PNG, made by the export worker from what the server has stored.
export default function ExportButtons({ documentId, title }: { documentId: string; title: string }) {
  const [busy, setBusy] = useState(false)
  const [progress, setProgress] = useState<string | null>(null)
  const [problem, setProblem] = useState<string | null>(null)
  // Stops waiting when the page closes. Made in the effect, so that each mount has its own: React mounts
  // twice in development, and a controller aborted by the first unmount would stop every export.
  const leaving = useRef<AbortController | null>(null)
  useEffect(() => {
    const controller = new AbortController()
    leaving.current = controller
    return () => controller.abort()
  }, [])

  async function download(format: ExportFormat) {
    setBusy(true)
    setProblem(null)
    const signal = leaving.current?.signal
    const result = await exportDrawing(documentId, format, { onProgress: setProgress, signal })
    if (signal?.aborted) return
    setBusy(false)
    setProgress(null)
    if ('file' in result) saveFile(result.file, fileName(title, format))
    else setProblem(result.problem)
  }

  return (
    <div className="exports">
      <button onClick={() => void download('svg')} disabled={busy}>Download SVG</button>
      <button onClick={() => void download('png')} disabled={busy}>Download PNG</button>
      <span className="hint" aria-live="polite">{progress}</span>
      {problem && <p className="error" role="alert">{problem}</p>}
    </div>
  )
}
