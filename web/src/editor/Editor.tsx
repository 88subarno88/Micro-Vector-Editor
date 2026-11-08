import { useState } from 'react'
import type { Core } from '../core/core'
import DrawingCanvas, { type PointerPhase } from './DrawingCanvas'
import type { DrawCommand } from './paint'
import { drawingTools, newShapeId, shapeForDrag, type DrawingTool, type Point } from './tools'
import { useDrawing, type LoadedDocument } from './useDrawing'

// The editor: a toolbar of drawing tools above the drawing. Edits change the drawing in this browser
// only, until edits are sent to the server (step 31).
export default function Editor({ core, loaded }: { core: Core; loaded: LoadedDocument }) {
  const { commands, error, ready, apply, preview } = useDrawing(core, loaded)
  const [tool, setTool] = useState<DrawingTool>('rect')
  const [drag, setDrag] = useState<{ id: string; start: Point } | null>(null)
  const [shown, setShown] = useState<DrawCommand[] | null>(null) // the drawing with the shape being drawn
  const [problem, setProblem] = useState<string | null>(null)

  function onPointer(phase: PointerPhase, point: Point) {
    if (phase === 'down') {
      setDrag({ id: newShapeId(), start: point })
      setProblem(null)
      return
    }
    if (!drag) return
    const op = shapeForDrag(tool, drag.start, point, drag.id)
    if (phase === 'move') {
      setShown(op ? preview(op) : null)
      return
    }
    setDrag(null)
    setShown(null)
    if (phase === 'up' && op) setProblem(apply(op))
  }

  if (error) return <p className="error" role="alert">{error}</p>
  if (!ready) return <p className="loading">Loading the drawing…</p>

  return (
    <section className="editor">
      <div className="toolbar" role="toolbar" aria-label="Tools">
        {drawingTools.map(({ tool: option, label }) => (
          <button key={option} aria-pressed={tool === option} onClick={() => setTool(option)}>
            {label}
          </button>
        ))}
      </div>
      <DrawingCanvas commands={shown ?? commands} onPointer={onPointer} />
      {problem && <p className="error" role="alert">That edit didn't apply: {problem}</p>}
      <p className="hint">Changes aren't saved yet: they stay in this browser tab until live editing arrives.</p>
    </section>
  )
}
