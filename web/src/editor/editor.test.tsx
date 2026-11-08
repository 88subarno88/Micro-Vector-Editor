// @vitest-environment jsdom
import '@testing-library/jest-dom/vitest'
import { cleanup, fireEvent, render, screen } from '@testing-library/react'
import userEvent from '@testing-library/user-event'
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest'
import type { Core } from '../core/core'
import Editor from './Editor'
import type { LoadedDocument } from './useDrawing'

// A stand-in for the core: keeps shapes by ID, refuses a duplicate ID or a shape starting past x = 700,
// and draws one rect command per shape. Records every op it's given.
function fakeCore() {
  const ops: { kind: string; id: string; shape?: { kind: string; x?: number } }[] = []
  const shapes = new Map<string, unknown>()
  const ok = '{"ok":true}'
  const refuse = (message: string) => JSON.stringify({ error: { code: 'invalid_value', message } })
  const core = {
    Document: class {
      applyOp(text: string) {
        const op = JSON.parse(text)
        ops.push(op)
        if (op.kind === 'addShape') {
          if (shapes.has(op.id)) return refuse('duplicate')
          if ((op.shape.x ?? op.shape.center?.x ?? 0) > 700) return refuse('out of range')
          shapes.set(op.id, op.shape)
        } else if (op.kind === 'deleteShape') {
          shapes.delete(op.id)
        }
        return ok
      }
      loadJson() { return ok }
      drawCommands() {
        return JSON.stringify([...shapes.keys()].map((id) => ({ id, kind: 'rect' })))
      }
      delete() {}
    },
  }
  return { core: core as unknown as Core, ops, shapes }
}

const loaded: LoadedDocument = {
  document: { id: 'd1', ownerId: 'u1', title: 'Plans', headSeq: 0, createdAt: 'x', updatedAt: 'x' },
  role: 'owner',
  snapshot: null,
  ops: [],
  opsComplete: true,
}

const canvas = () => screen.getByRole('img', { name: 'The drawing' })
function drag(from: [number, number], to: [number, number]) {
  fireEvent.pointerDown(canvas(), { clientX: from[0], clientY: from[1], pointerId: 1 })
  fireEvent.pointerMove(canvas(), { clientX: (from[0] + to[0]) / 2, clientY: (from[1] + to[1]) / 2, pointerId: 1 })
  fireEvent.pointerMove(canvas(), { clientX: to[0], clientY: to[1], pointerId: 1 })
  fireEvent.pointerUp(canvas(), { clientX: to[0], clientY: to[1], pointerId: 1 })
}

beforeEach(() => {
  vi.spyOn(HTMLCanvasElement.prototype, 'getContext').mockReturnValue(null)
  // Shown at half size, so pointer positions must be scaled up to drawing units.
  vi.spyOn(HTMLCanvasElement.prototype, 'getBoundingClientRect').mockReturnValue({
    left: 0, top: 0, width: 400, height: 300, right: 400, bottom: 300, x: 0, y: 0, toJSON: () => ({}),
  })
})
afterEach(() => {
  cleanup()
  vi.restoreAllMocks()
})

describe('the editor', () => {
  it('starts with the rectangle tool', () => {
    render(<Editor core={fakeCore().core} loaded={loaded} />)
    expect(screen.getByRole('button', { name: 'Rectangle' })).toHaveAttribute('aria-pressed', 'true')
  })

  it('adds one shape per drag, in drawing units, however big the canvas is shown', () => {
    const { core, ops, shapes } = fakeCore()
    render(<Editor core={core} loaded={loaded} />)

    drag([50, 50], [100, 75])

    const kept = [...shapes.values()]
    expect(kept).toEqual([expect.objectContaining({ kind: 'rect', x: 100, y: 100, width: 100, height: 50 })])
    // Each move previewed the shape by adding and removing it; only the final add stayed.
    expect(ops.map((op) => op.kind)).toEqual(['addShape', 'deleteShape', 'addShape', 'deleteShape', 'addShape'])
    expect(new Set(ops.map((op) => op.id)).size).toBe(1)
  })

  it('draws with the chosen tool', async () => {
    const { core, shapes } = fakeCore()
    render(<Editor core={core} loaded={loaded} />)
    await userEvent.click(screen.getByRole('button', { name: 'Circle' }))
    expect(screen.getByRole('button', { name: 'Circle' })).toHaveAttribute('aria-pressed', 'true')

    drag([10, 10], [60, 30])

    expect([...shapes.values()]).toEqual([expect.objectContaining({ kind: 'circle', radius: 50 })])
  })

  it('gives every shape its own ID', () => {
    const { core, shapes } = fakeCore()
    render(<Editor core={core} loaded={loaded} />)
    drag([10, 10], [60, 30])
    drag([10, 10], [60, 30])
    expect(shapes.size).toBe(2)
  })

  it('ignores a click without a drag', () => {
    const { core, ops } = fakeCore()
    render(<Editor core={core} loaded={loaded} />)
    fireEvent.pointerDown(canvas(), { clientX: 10, clientY: 10, pointerId: 1 })
    fireEvent.pointerUp(canvas(), { clientX: 10, clientY: 10, pointerId: 1 })
    expect(ops).toEqual([])
  })

  it('says why an edit was refused', async () => {
    const { core, shapes } = fakeCore()
    render(<Editor core={core} loaded={loaded} />)
    drag([360, 10], [390, 30]) // starts at x = 720 in drawing units
    expect(await screen.findByRole('alert')).toHaveTextContent("That edit didn't apply: out of range")
    expect(shapes.size).toBe(0)
  })

  it("says that changes aren't saved yet", () => {
    render(<Editor core={fakeCore().core} loaded={loaded} />)
    expect(screen.getByText(/Changes aren't saved yet/)).toBeInTheDocument()
  })
})
