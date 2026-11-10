// @vitest-environment jsdom
import '@testing-library/jest-dom/vitest'
import { act, cleanup, fireEvent, render, screen } from '@testing-library/react'
import userEvent from '@testing-library/user-event'
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest'
import type { Core } from '../core/core'
import { FakeSocket } from '../sync/testing'
import Editor from './Editor'
import type { LoadedDocument, SyncOverrides } from './useDrawing'

// A stand-in for the core: keeps shapes by ID, refuses a duplicate ID or a shape starting past x = 700,
// moves rectangles, hit-tests and outlines them by their box, inverts adds and moves, and draws one rect
// command per shape. Each document has its own shapes; `shapes` and `ops` are those of the first one made,
// which is the drawing the editor shows (the second is the server's, as this client knows it).
// The box around a fake shape: a rectangle's own, or the one around a line's two ends.
function boxOf(shape: Record<string, unknown>) {
  if (shape.start && shape.end) {
    const [a, b] = [shape.start, shape.end] as { x: number; y: number }[]
    return { x: Math.min(a.x, b.x), y: Math.min(a.y, b.y), width: Math.abs(a.x - b.x), height: Math.abs(a.y - b.y) }
  }
  const r = shape as { x: number; y: number; width: number; height: number }
  return { x: r.x, y: r.y, width: r.width, height: r.height }
}

type FakeOp = { kind: string; id: string; shape?: Record<string, unknown> & { kind: string } }

function fakeCore() {
  const shown = { ops: [] as FakeOp[], shapes: new Map<string, Record<string, unknown>>() }
  let made = 0
  const ok = '{"ok":true}'
  const refuse = (message: string) => JSON.stringify({ error: { code: 'invalid_value', message } })
  const core = {
    Document: class {
      ops: FakeOp[]
      shapes: Map<string, Record<string, unknown>>
      constructor() {
        const own = made++ === 0 ? shown : { ops: [], shapes: new Map() }
        this.ops = own.ops
        this.shapes = own.shapes
      }
      applyOp(text: string) {
        const { ops, shapes } = this
        const op = JSON.parse(text)
        ops.push(op)
        if (op.kind === 'addShape') {
          if (shapes.has(op.id)) return refuse('duplicate')
          if ((op.shape.x ?? op.shape.center?.x ?? op.shape.position?.x ?? 0) > 700) return refuse('out of range')
          shapes.set(op.id, op.shape)
        } else if (op.kind === 'deleteShape') {
          shapes.delete(op.id)
        } else if (op.kind === 'move') {
          const shape = shapes.get(op.id) as { x: number; y: number }
          shape.x += op.dx
          shape.y += op.dy
        } else if (op.kind === 'setStyle') {
          const shape = shapes.get(op.id) as { style: Record<string, unknown> }
          for (const key of ['stroke', 'fill', 'strokeWidth']) if (key in op) shape.style = { ...shape.style, [key]: op[key] }
        } else if (op.kind === 'resize') {
          // Like the core's rectangle: grows from its top left.
          const shape = shapes.get(op.id) as { width: number; height: number }
          shape.width *= op.factor
          shape.height *= op.factor
        }
        return ok
      }
      invertOp(text: string) {
        const { shapes } = this
        const op = JSON.parse(text)
        if (op.kind === 'addShape') return JSON.stringify({ op: { kind: 'deleteShape', id: op.id } })
        if (op.kind === 'move') return JSON.stringify({ op: { ...op, dx: -op.dx, dy: -op.dy } })
        if (op.kind === 'resize') return JSON.stringify({ op: { ...op, factor: 1 / op.factor } })
        if (op.kind === 'setStyle') {
          const before = (shapes.get(op.id) as { style: Record<string, unknown> }).style
          const inverse: Record<string, unknown> = { kind: 'setStyle', id: op.id }
          for (const key of ['stroke', 'fill', 'strokeWidth']) if (key in op) inverse[key] = before[key]
          return JSON.stringify({ op: inverse })
        }
        return refuse('no inverse')
      }
      hitTest(x: number, y: number) {
        const hits = [...this.shapes].filter(([, s]) => {
          const r = boxOf(s)
          return x >= r.x && x <= r.x + r.width && y >= r.y && y <= r.y + r.height
        })
        return hits.length ? hits[hits.length - 1][0] : ''
      }
      boundsOf(id: string) {
        const shape = this.shapes.get(id)
        return shape ? JSON.stringify(boxOf(shape)) : 'null'
      }
      loadJson(text: string) {
        this.shapes.clear()
        for (const { id, shape } of JSON.parse(text).shapes ?? []) this.shapes.set(id, shape)
        return ok
      }
      drawCommands() {
        return JSON.stringify([...this.shapes].map(([id, shape]) => ({
          id,
          kind: shape.kind === 'line' || shape.kind === 'freehand' ? 'polyline' : 'rect',
          closed: false,
          style: shape.style,
        })))
      }
      shapesJson() {
        return JSON.stringify({ shapes: [...this.shapes].map(([id, shape]) => ({ id, shape })) })
      }
      translatedShape(text: string, dx: number, dy: number) {
        const shape = JSON.parse(text)
        return JSON.stringify({ shape: { ...shape, x: shape.x + dx, y: shape.y + dy } })
      }
      delete() {}
    },
  }
  return { core: core as unknown as Core, ops: shown.ops, shapes: shown.shapes }
}

const loaded: LoadedDocument = {
  document: { id: 'd1', ownerId: 'u1', title: 'Plans', headSeq: 0, createdAt: 'x', updatedAt: 'x' },
  role: 'owner',
  snapshot: null,
  ops: [],
  opsComplete: true,
}

const canvas = () => screen.getByRole('img', { name: 'The drawing' })
const chooseTool = (name: string) => userEvent.click(screen.getByRole('button', { name }))
const click = (at: [number, number]) => {
  fireEvent.pointerDown(canvas(), { clientX: at[0], clientY: at[1], pointerId: 1 })
  fireEvent.pointerUp(canvas(), { clientX: at[0], clientY: at[1], pointerId: 1 })
}
const deleteButton = () => screen.getByRole('button', { name: 'Delete shape' })
function drag(from: [number, number], to: [number, number]) {
  fireEvent.pointerDown(canvas(), { clientX: from[0], clientY: from[1], pointerId: 1 })
  fireEvent.pointerMove(canvas(), { clientX: (from[0] + to[0]) / 2, clientY: (from[1] + to[1]) / 2, pointerId: 1 })
  fireEvent.pointerMove(canvas(), { clientX: to[0], clientY: to[1], pointerId: 1 })
  fireEvent.pointerUp(canvas(), { clientX: to[0], clientY: to[1], pointerId: 1 })
}

// The editor's connection to the server, which a test can play through `sockets`. Never opened unless a
// test opens it, so edits wait to be sent.
let sockets: FakeSocket[]
let sync: SyncOverrides

beforeEach(() => {
  sockets = []
  sync = {
    getToken: () => 'tok',
    renewToken: async () => true,
    openSocket: (url) => {
      const socket = new FakeSocket(url)
      sockets.push(socket)
      return socket
    },
  }
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
  it('starts with the select tool', () => {
    render(<Editor core={fakeCore().core} loaded={loaded} me="me" sync={sync} />)
    expect(screen.getByRole('button', { name: 'Select' })).toHaveAttribute('aria-pressed', 'true')
  })

  it('adds one shape per drag, in drawing units, however big the canvas is shown', async () => {
    const { core, ops, shapes } = fakeCore()
    render(<Editor core={core} loaded={loaded} me="me" sync={sync} />)
    await chooseTool('Rectangle')

    drag([50, 50], [100, 75])

    const kept = [...shapes.values()]
    expect(kept).toEqual([expect.objectContaining({ kind: 'rect', x: 100, y: 100, width: 100, height: 50 })])
    // Each move previewed the shape by adding and removing it; only the final add stayed.
    expect(ops.map((op) => op.kind)).toEqual(['addShape', 'deleteShape', 'addShape', 'deleteShape', 'addShape'])
    expect(new Set(ops.map((op) => op.id)).size).toBe(1)
  })

  it('draws with the chosen tool', async () => {
    const { core, shapes } = fakeCore()
    render(<Editor core={core} loaded={loaded} me="me" sync={sync} />)
    await userEvent.click(screen.getByRole('button', { name: 'Circle' }))
    expect(screen.getByRole('button', { name: 'Circle' })).toHaveAttribute('aria-pressed', 'true')

    drag([10, 10], [60, 30])

    expect([...shapes.values()]).toEqual([expect.objectContaining({ kind: 'circle', radius: 50 })])
  })

  it('gives every shape its own ID', async () => {
    const { core, shapes } = fakeCore()
    render(<Editor core={core} loaded={loaded} me="me" sync={sync} />)
    await chooseTool('Rectangle')
    drag([10, 10], [60, 30])
    drag([10, 10], [60, 30])
    expect(shapes.size).toBe(2)
  })

  it('ignores a click without a drag', async () => {
    const { core, ops } = fakeCore()
    render(<Editor core={core} loaded={loaded} me="me" sync={sync} />)
    await chooseTool('Rectangle')
    fireEvent.pointerDown(canvas(), { clientX: 10, clientY: 10, pointerId: 1 })
    fireEvent.pointerUp(canvas(), { clientX: 10, clientY: 10, pointerId: 1 })
    expect(ops).toEqual([])
  })

  it('says why an edit was refused', async () => {
    const { core, shapes } = fakeCore()
    render(<Editor core={core} loaded={loaded} me="me" sync={sync} />)
    await chooseTool('Rectangle')
    drag([360, 10], [390, 30]) // starts at x = 720 in drawing units
    expect(await screen.findByRole('alert')).toHaveTextContent("That edit didn't apply: out of range")
    expect(shapes.size).toBe(0)
  })

  it('draws a hexagon', async () => {
    const { core, shapes } = fakeCore()
    render(<Editor core={core} loaded={loaded} me="me" sync={sync} />)
    await userEvent.click(screen.getByRole('button', { name: 'Hexagon' }))
    drag([0, 0], [50, 20])
    expect([...shapes.values()]).toEqual([expect.objectContaining({ kind: 'hexagon', center: { x: 50, y: 20 }, radius: 20 })])
  })

  it('keeps every point of a freehand stroke, in order', async () => {
    const { core, shapes } = fakeCore()
    render(<Editor core={core} loaded={loaded} me="me" sync={sync} />)
    await userEvent.click(screen.getByRole('button', { name: 'Freehand' }))

    drag([10, 10], [30, 20]) // down, two moves, up

    expect([...shapes.values()][0]).toMatchObject({
      kind: 'freehand',
      points: [{ x: 20, y: 20 }, { x: 40, y: 30 }, { x: 60, y: 40 }, { x: 60, y: 40 }],
    })
  })

  describe('text', () => {
    async function clickWithText(at: [number, number]) {
      await userEvent.click(screen.getByRole('button', { name: 'Text' }))
      fireEvent.pointerDown(canvas(), { clientX: at[0], clientY: at[1], pointerId: 1 })
      fireEvent.pointerUp(canvas(), { clientX: at[0], clientY: at[1], pointerId: 1 })
      return screen.getByLabelText('Text')
    }

    it('asks for text where you click, and adds it on Enter', async () => {
      const { core, shapes } = fakeCore()
      render(<Editor core={core} loaded={loaded} me="me" sync={sync} />)

      const entry = await clickWithText([200, 150]) // the middle of the canvas, shown at half size
      expect(entry).toHaveStyle({ left: '50%', top: '50%' })
      await userEvent.type(entry, 'Hello{Enter}')

      expect([...shapes.values()]).toEqual([expect.objectContaining({ kind: 'text', text: 'Hello', position: { x: 400, y: 300 }, fontSize: 16 })])
      expect(screen.queryByLabelText('Text')).not.toBeInTheDocument()
    })

    it('keeps what was typed when you click away', async () => {
      const { core, shapes } = fakeCore()
      render(<Editor core={core} loaded={loaded} me="me" sync={sync} />)
      await userEvent.type(await clickWithText([10, 10]), 'Kept')
      fireEvent.blur(screen.getByLabelText('Text'))
      expect([...shapes.values()]).toEqual([expect.objectContaining({ text: 'Kept' })])
    })

    it('adds nothing on Escape, or for empty text', async () => {
      const { core, shapes } = fakeCore()
      render(<Editor core={core} loaded={loaded} me="me" sync={sync} />)
      await userEvent.type(await clickWithText([10, 10]), 'Gone{Escape}')
      await userEvent.type(await clickWithText([10, 10]), '{Enter}')
      expect(shapes.size).toBe(0)
    })
  })

  describe('selecting', () => {
    // A rectangle from (100, 100) to (200, 200) in drawing units: (50, 50) to (100, 100) on screen.
    async function withRectangle() {
      const fake = fakeCore()
      render(<Editor core={fake.core} loaded={loaded} me="me" sync={sync} />)
      await chooseTool('Rectangle')
      drag([50, 50], [100, 100])
      await chooseTool('Select')
      fake.ops.length = 0
      return { ...fake, id: [...fake.shapes.keys()][0] }
    }

    it('selects the shape clicked, and nothing when clicking empty space', async () => {
      await withRectangle()
      expect(deleteButton()).toBeDisabled()
      click([75, 75])
      expect(deleteButton()).toBeEnabled()
      click([300, 250])
      expect(deleteButton()).toBeDisabled()
    })

    it('moves the shape dragged, with one move op in drawing units', async () => {
      const { ops, shapes, id } = await withRectangle()

      drag([75, 75], [95, 85]) // 20 by 10 on screen

      expect(shapes.get(id)).toMatchObject({ x: 140, y: 120 })
      expect(ops.filter((op) => op.kind === 'move').at(-1)).toEqual({ kind: 'move', id, dx: 40, dy: 20 })
      // Each preview was undone, so all the moves before the last cancel out.
      const total = ops.reduce((sum, op) => sum + ((op as { dx?: number }).dx ?? 0), 0)
      expect(total).toBe(40)
    })

    it('resizes the shape by dragging its handle, with one resize op', async () => {
      const { ops, shapes, id } = await withRectangle()
      click([75, 75])
      ops.length = 0

      // The handle is at the corner farthest from where the rectangle grows from: (200, 200), so (100, 100)
      // on screen. Dragging it to (125, 125) on screen, (250, 250), scales the rectangle by 1.5. (Not 2:
      // selecting also resizes by 2 and back, to measure where the shape grows from.)
      drag([100, 100], [125, 125])

      expect(shapes.get(id)).toMatchObject({ x: 100, y: 100, width: 150, height: 150 })
      expect(ops).toContainEqual({ kind: 'resize', id, factor: 1.5 })
    })

    it('still moves the shape when it is pressed away from the handle', async () => {
      const { shapes, id } = await withRectangle()
      click([75, 75])
      drag([60, 60], [70, 60])
      expect(shapes.get(id)).toMatchObject({ x: 120, y: 100, width: 100 })
    })

    it("doesn't resize on a click on the handle", async () => {
      const { ops, shapes, id } = await withRectangle()
      click([75, 75])
      ops.length = 0
      click([100, 100])
      expect(shapes.get(id)).toMatchObject({ width: 100, height: 100 })
    })

    it('measures where a shape grows from without changing it', async () => {
      const { ops, shapes, id } = await withRectangle()
      click([75, 75])
      // Selecting measured the anchor: a resize by 2 and straight back.
      const probes = ops.filter((op) => op.kind === 'resize').map((op) => (op as { factor?: number }).factor)
      expect(probes.length).toBeGreaterThan(0)
      expect(probes.every((f, i) => f === (i % 2 === 0 ? 2 : 0.5))).toBe(true)
      expect(shapes.get(id)).toMatchObject({ x: 100, y: 100, width: 100, height: 100 })
    })

    it("doesn't move a shape that was only clicked", async () => {
      const { ops } = await withRectangle()
      click([75, 75])
      expect(ops.filter((op) => op.kind === 'move')).toEqual([])
    })

    it('deletes the selected shape with Delete, Backspace or the button', async () => {
      for (const remove of [() => userEvent.keyboard('{Delete}'), () => userEvent.keyboard('{Backspace}'), () => userEvent.click(deleteButton())]) {
        cleanup()
        const { shapes } = await withRectangle()
        click([75, 75])
        await remove()
        expect(shapes.size).toBe(0)
        expect(deleteButton()).toBeDisabled()
      }
    })

    it("doesn't delete the shape when Backspace is pressed while typing elsewhere", async () => {
      const fake = fakeCore()
      render(
        <>
          <input aria-label="Somewhere else" />
          <Editor core={fake.core} loaded={loaded} me="me" sync={sync} />
        </>,
      )
      await chooseTool('Rectangle')
      drag([50, 50], [100, 100])
      await chooseTool('Select')
      click([75, 75])
      expect(deleteButton()).toBeEnabled()

      await userEvent.type(screen.getByLabelText('Somewhere else'), 'ab{Backspace}')

      expect(screen.getByLabelText('Somewhere else')).toHaveValue('a')
      expect(fake.shapes.size).toBe(1)
    })

    it('clears the selection when a drawing tool is chosen', async () => {
      await withRectangle()
      click([75, 75])
      await chooseTool('Circle')
      expect(deleteButton()).toBeDisabled()
    })
  })

  describe('the style panel', () => {
    async function withSelected(toolName = 'Rectangle') {
      const fake = fakeCore()
      render(<Editor core={fake.core} loaded={loaded} me="me" sync={sync} />)
      await chooseTool(toolName)
      drag([50, 50], [100, 100])
      await chooseTool('Select')
      click([75, 75])
      fake.ops.length = 0
      const id = [...fake.shapes.keys()][0]
      const styleOf = () => (fake.shapes.get(id) as { style: Record<string, unknown> }).style
      const committed = () => fake.ops.filter((op) => op.kind === 'setStyle')
      return { ...fake, id, styleOf, committed }
    }
    const width = () => screen.getByLabelText('Width')

    it("shows only with a shape selected, with that shape's style", async () => {
      render(<Editor core={fakeCore().core} loaded={loaded} me="me" sync={sync} />)
      expect(screen.queryByRole('group', { name: 'Style' })).not.toBeInTheDocument()
      cleanup()

      await withSelected()
      expect(screen.getByLabelText('Stroke')).toHaveValue('#000000')
      expect(screen.getByLabelText('Fill')).toHaveValue('#ffffff')
      expect(width()).toHaveValue(1)
    })

    it('has no fill for a line', async () => {
      await withSelected('Line')
      expect(screen.getByRole('group', { name: 'Style' })).toBeInTheDocument()
      expect(screen.queryByLabelText('Fill')).not.toBeInTheDocument()
    })

    it('sets the stroke width with one edit, on Enter', async () => {
      const { styleOf, committed, id } = await withSelected()
      await userEvent.clear(width())
      await userEvent.type(width(), '4{Enter}')
      expect(styleOf()).toMatchObject({ strokeWidth: 4 })
      expect(committed().at(-1)).toEqual({ kind: 'setStyle', id, strokeWidth: 4 })
      expect(committed().filter((op) => (op as { strokeWidth?: number }).strokeWidth === 4)).toHaveLength(2) // a preview, then the edit
    })

    it('changes nothing on Escape, or for a width out of range', async () => {
      const { styleOf } = await withSelected()
      await userEvent.clear(width())
      await userEvent.type(width(), '7{Escape}')
      expect(styleOf()).toMatchObject({ strokeWidth: 1 })
      await userEvent.clear(width())
      await userEvent.type(width(), '99{Enter}')
      expect(styleOf()).toMatchObject({ strokeWidth: 1 })
      expect(width()).toHaveValue(1)
    })

    it('previews a colour while the picker moves, and commits it when the picker closes', async () => {
      const { styleOf } = await withSelected()
      fireEvent.input(screen.getByLabelText('Stroke'), { target: { value: '#ff0000' } })
      expect(styleOf()).toMatchObject({ stroke: 'black' }) // only previewed
      fireEvent.change(screen.getByLabelText('Stroke'), { target: { value: '#ff0000' } })
      expect(styleOf()).toMatchObject({ stroke: '#ff0000' })
    })

    it('turns the fill off and back on, keeping its colour', async () => {
      const { styleOf } = await withSelected()
      await userEvent.click(screen.getByLabelText('No fill'))
      expect(styleOf()).toMatchObject({ fill: 'none' })
      expect(screen.getByLabelText('Fill')).toBeDisabled()
      await userEvent.click(screen.getByLabelText('No fill'))
      expect(styleOf()).toMatchObject({ fill: '#ffffff' })
    })

    it("doesn't delete the shape when Backspace is pressed in the width", async () => {
      const { shapes } = await withSelected()
      await userEvent.type(width(), '{Backspace}')
      expect(shapes.size).toBe(1)
    })
  })

  describe('shortcuts', () => {
    // A rectangle at (100, 100), 100 by 100, selected.
    async function withSelected() {
      const fake = fakeCore()
      render(
        <>
          <input aria-label="Somewhere else" />
          <Editor core={fake.core} loaded={loaded} me="me" sync={sync} />
        </>,
      )
      await chooseTool('Rectangle')
      drag([50, 50], [100, 100])
      await chooseTool('Select')
      click([75, 75])
      const id = [...fake.shapes.keys()][0]
      const others = () => [...fake.shapes].filter(([key]) => key !== id).map(([, shape]) => shape)
      return { ...fake, id, others }
    }

    it('copies and pastes beside the original, selecting the copy', async () => {
      const { shapes, others } = await withSelected()
      await userEvent.keyboard('{Control>}c{/Control}{Control>}v{/Control}')
      expect(others()).toEqual([expect.objectContaining({ x: 120, y: 120, width: 100 })])
      await userEvent.keyboard('{Delete}') // the copy is selected, so this removes it
      expect(shapes.size).toBe(1)
    })

    it('steps each paste further along, instead of piling them up', async () => {
      const { others } = await withSelected()
      await userEvent.keyboard('{Control>}c{/Control}{Control>}v{/Control}{Control>}v{/Control}')
      expect(others().map((shape) => shape.x)).toEqual([120, 140])
    })

    it('cuts, then pastes beside where the shape was', async () => {
      const { shapes, id, others } = await withSelected()
      await userEvent.keyboard('{Control>}x{/Control}')
      expect(shapes.has(id)).toBe(false)
      await userEvent.keyboard('{Control>}v{/Control}')
      expect(others()).toEqual([expect.objectContaining({ x: 120, y: 120 })])
    })

    it('nudges the selected shape with the arrows, further with Shift', async () => {
      const { shapes, id } = await withSelected()
      await userEvent.keyboard('{ArrowRight}{Shift>}{ArrowDown}{/Shift}')
      expect(shapes.get(id)).toMatchObject({ x: 101, y: 110 })
    })

    it('deselects with Escape', async () => {
      await withSelected()
      await userEvent.keyboard('{Escape}')
      expect(deleteButton()).toBeDisabled()
    })

    it("chooses tools by letter, but not while typing", async () => {
      await withSelected()
      await userEvent.keyboard('r')
      expect(screen.getByRole('button', { name: 'Rectangle' })).toHaveAttribute('aria-pressed', 'true')
      await userEvent.type(screen.getByLabelText('Somewhere else'), 'v')
      expect(screen.getByRole('button', { name: 'Rectangle' })).toHaveAttribute('aria-pressed', 'true')
      screen.getByLabelText('Somewhere else').blur() // done typing
      await userEvent.keyboard('v')
      expect(screen.getByRole('button', { name: 'Select' })).toHaveAttribute('aria-pressed', 'true')
    })

    it("doesn't copy while typing", async () => {
      const { shapes } = await withSelected()
      await userEvent.type(screen.getByLabelText('Somewhere else'), '{Control>}c{/Control}{Control>}v{/Control}')
      expect(shapes.size).toBe(1)
    })
  })

  describe('zoom and pan', () => {
    const zoomLabel = () => screen.getByRole('button', { name: 'Reset zoom' })

    it('zooms in and out by the buttons and keys, and resets', async () => {
      render(<Editor core={fakeCore().core} loaded={loaded} me="me" sync={sync} />)
      expect(zoomLabel()).toHaveTextContent('100%')
      await userEvent.click(screen.getByRole('button', { name: 'Zoom in' }))
      expect(zoomLabel()).toHaveTextContent('125%')
      await userEvent.keyboard('{Control>}={/Control}')
      expect(zoomLabel()).toHaveTextContent('156%')
      await userEvent.click(screen.getByRole('button', { name: 'Zoom out' }))
      await userEvent.click(zoomLabel())
      expect(zoomLabel()).toHaveTextContent('100%')
    })

    it('draws where the pointer is in the drawing, however it is zoomed', async () => {
      const { core, shapes } = fakeCore()
      render(<Editor core={core} loaded={loaded} me="me" sync={sync} />)
      // Zoomed to 125% around the canvas's centre, (400, 300): its top left now shows (80, 60).
      await userEvent.click(screen.getByRole('button', { name: 'Zoom in' }))
      await chooseTool('Rectangle')
      drag([50, 50], [100, 100]) // (100, 100) to (200, 200) on the canvas, before zoom

      expect([...shapes.values()][0]).toMatchObject({ x: 160, y: 140, width: 80, height: 80 })
    })

    it('pans with the wheel, and zooms with Ctrl and the wheel', async () => {
      const { core, shapes } = fakeCore()
      render(<Editor core={core} loaded={loaded} me="me" sync={sync} />)
      fireEvent.wheel(canvas(), { deltaY: 100 })
      await chooseTool('Rectangle')
      drag([50, 50], [100, 100])
      expect([...shapes.values()][0]).toMatchObject({ x: 100, y: 200 }) // the view moved down by 100

      fireEvent.wheel(canvas(), { deltaY: -100, ctrlKey: true, clientX: 200, clientY: 150 })
      expect(zoomLabel()).toHaveTextContent('125%')
    })
  })

  describe('with the server', () => {
    // Plays the server up to joined, with an empty drawing.
    function goLive() {
      const socket = sockets[0]
      act(() => {
        socket.opened()
        socket.receive({ type: 'ready', role: 'editor' })
        socket.receive({ type: 'joined', headSeq: 0, ops: [] })
      })
      return socket
    }
    const opsSent = (socket: FakeSocket) =>
      socket.sent.filter((m) => (m as { type: string }).type === 'op') as { clientOpId: string; op: Record<string, unknown> }[]

    it('sends each edit', async () => {
      const { core } = fakeCore()
      render(<Editor core={core} loaded={loaded} me="me" sync={sync} />)
      const socket = goLive()
      await chooseTool('Rectangle')
      drag([50, 50], [100, 75])
      expect(opsSent(socket).map((m) => m.op)).toEqual([expect.objectContaining({ kind: 'addShape' })])
    })

    it('shows edits others make', () => {
      const { core, shapes } = fakeCore()
      render(<Editor core={core} loaded={loaded} me="me" sync={sync} />)
      const socket = goLive()
      const shape = { kind: 'rect', x: 1, y: 2, width: 3, height: 4, style: {} }
      act(() => socket.receive({ type: 'op', seq: 1, userId: 'bob', clientOpId: 'b1', op: { kind: 'addShape', id: 'b', shape } }))
      expect(shapes.get('b')).toEqual(shape)
    })

    it('takes back an edit the server refuses, and says why', async () => {
      const { core, shapes } = fakeCore()
      render(<Editor core={core} loaded={loaded} me="me" sync={sync} />)
      const socket = goLive()
      await chooseTool('Rectangle')
      drag([50, 50], [100, 75])
      expect(shapes.size).toBe(1)

      const { clientOpId } = opsSent(socket)[0]
      act(() => socket.receive({ type: 'reject', clientOpId, reason: 'duplicate_id', message: 'id taken' }))

      expect(shapes.size).toBe(0)
      expect(screen.getByRole('alert')).toHaveTextContent(
        "That edit wasn't saved: a shape with the same ID was added at the same moment.",
      )
    })

    it("draws the others' pointers where they are, with their names, but not this user's", () => {
      render(<Editor core={fakeCore().core} loaded={loaded} me="me" sync={sync} />)
      const socket = goLive()
      act(() => socket.receive({
        type: 'presence',
        users: [
          { id: 'me', name: 'me@example.com', cursor: { x: 10, y: 10 } },
          { id: 'u2', name: 'bob@example.com', cursor: { x: 400, y: 150 } },
          { id: 'u3', name: 'cy@example.com', cursor: null }, // hasn't moved yet
        ],
      }))

      const bob = screen.getByRole('img', { name: "bob@example.com's pointer" })
      expect(bob).toHaveStyle({ left: '50%', top: '25%' })
      expect(bob).toHaveTextContent('bob@example.com')
      expect(screen.queryByRole('img', { name: "me@example.com's pointer" })).not.toBeInTheDocument()
      expect(screen.queryByRole('img', { name: "cy@example.com's pointer" })).not.toBeInTheDocument()

      expect(screen.getByLabelText('bob@example.com')).toHaveTextContent('B') // and who's here
      expect(screen.getByLabelText('me@example.com (you)')).toBeInTheDocument()

      act(() => socket.receive({ type: 'presence', users: [] })) // bob left
      expect(screen.queryByRole('img', { name: "bob@example.com's pointer" })).not.toBeInTheDocument()
      expect(screen.queryByLabelText('bob@example.com')).not.toBeInTheDocument()
    })

    it("leaves out a pointer that's outside the part of the drawing in view", async () => {
      render(<Editor core={fakeCore().core} loaded={loaded} me="me" sync={sync} />)
      const socket = goLive()
      await userEvent.click(screen.getByRole('button', { name: 'Zoom in' })) // the corners are now out of view
      act(() => socket.receive({ type: 'presence', users: [{ id: 'u2', name: 'bob', cursor: { x: 10, y: 10 } }] }))
      expect(screen.queryByRole('img', { name: "bob's pointer" })).not.toBeInTheDocument()
    })

    it("sends this user's pointer in drawing units, at most once every 100 ms", () => {
      render(<Editor core={fakeCore().core} loaded={loaded} me="me" sync={sync} />)
      const socket = goLive()
      vi.useFakeTimers()
      const cursors = () => socket.sent.filter((m) => (m as { type: string }).type === 'cursor')
      for (const x of [10, 20, 30]) fireEvent.pointerMove(canvas(), { clientX: x, clientY: 5, pointerId: 1 })
      expect(cursors()).toEqual([{ type: 'cursor', x: 20, y: 10 }]) // shown at half size

      act(() => vi.advanceTimersByTime(100))
      expect(cursors()).toEqual([{ type: 'cursor', x: 20, y: 10 }, { type: 'cursor', x: 60, y: 10 }])
      vi.useRealTimers()
    })

    it("sends a dragged shape's move every 50 ms while it moves, and the rest when it's let go", async () => {
      const { core, shapes } = fakeCore()
      // A canvas that records only the boxes it strokes: the last is the selected shape's dashed outline.
      const stroked: number[][] = []
      const context = new Proxy({}, {
        get: (_, key) => (key === 'strokeRect' ? (...box: number[]) => stroked.push(box) : () => {}),
        set: () => true,
      })
      vi.spyOn(HTMLCanvasElement.prototype, 'getContext').mockReturnValue(context as CanvasRenderingContext2D)
      render(<Editor core={core} loaded={loaded} me="me" sync={sync} />)
      const socket = goLive()
      await chooseTool('Rectangle')
      drag([50, 50], [100, 100]) // (100, 100) to (200, 200) in drawing units
      await chooseTool('Select')
      const id = [...shapes.keys()][0]
      vi.useFakeTimers()
      const moves = () => opsSent(socket).filter((m) => m.op.kind === 'move').map((m) => m.op)
      const pointer = (kind: 'pointerDown' | 'pointerMove' | 'pointerUp', x: number) =>
        fireEvent[kind](canvas(), { clientX: x, clientY: 75, pointerId: 1 })

      pointer('pointerDown', 75)
      pointer('pointerMove', 80)
      pointer('pointerMove', 85)
      expect(moves()).toEqual([]) // not one per pointer event
      act(() => vi.advanceTimersByTime(50))
      expect(moves()).toEqual([{ kind: 'move', id, dx: 20, dy: 0 }]) // 10 on screen, at half size
      expect(shapes.get(id)).toMatchObject({ x: 120 }) // others see it there too
      expect(stroked.at(-1)?.[0]).toBe(120 - 4) // the outline, 4 outside the shape, is drawn there once

      pointer('pointerMove', 90)
      pointer('pointerUp', 92)

      expect(moves()).toEqual([{ kind: 'move', id, dx: 20, dy: 0 }, { kind: 'move', id, dx: 14, dy: 0 }])
      expect(shapes.get(id)).toMatchObject({ x: 134, y: 100 })
      act(() => vi.advanceTimersByTime(1000))
      expect(moves()).toHaveLength(2)
      vi.useRealTimers()
    })
  })
})
