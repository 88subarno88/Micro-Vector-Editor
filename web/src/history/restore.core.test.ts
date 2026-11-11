// restoreOps with the real WebAssembly core: a random drawing, and a version of it a few random edits away,
// as a restore usually is. The ops that turn one into the other must leave exactly the other, drawing order
// included. Needs the core copied in with
// `npm run wasm`; skipped without it.
import { describe, expect, it } from 'vitest'
import { restoreOps } from './restore'

const loadCore = Object.values(
  import.meta.glob<{ default: () => Promise<{ Document: new () => CoreDocument }> }>('../wasm/vec.js'),
)[0]

type CoreDocument = {
  applyOp(op: string): string
  loadJson(json: string): string
  shapesJson(): string
  delete(): void
}

function random(seed: number) {
  return () => {
    seed = (seed + 0x6d2b79f5) | 0
    let t = Math.imul(seed ^ (seed >>> 15), 1 | seed)
    t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296
  }
}

// Random edits to a few shape IDs: adds, deletes, moves, restyles, adds at a position, and moving an
// unchanged shape to the bottom, which only the drawing order tells apart.
function edit(doc: CoreDocument, next: () => number, steps: number) {
  for (let i = 0; i < steps; i++) {
    if (next() < 0.2) {
      const shapes: { id: string; shape: object }[] = JSON.parse(doc.shapesJson()).shapes
      const { id, shape } = shapes[Math.floor(next() * shapes.length)] ?? {}
      if (id) {
        doc.applyOp(JSON.stringify({ kind: 'deleteShape', id }))
        doc.applyOp(JSON.stringify({ kind: 'addShape', id, shape, position: 0 }))
        reordered++
      }
      continue
    }
    const id = `s${Math.floor(next() * 6)}`
    const fill = ['white', 'red', 'none'][Math.floor(next() * 3)]
    const shape = {
      kind: 'rect',
      x: Math.floor(next() * 50),
      y: 0,
      width: 10,
      height: 10,
      cornerRadius: 0,
      style: { stroke: 'black', fill, strokeWidth: 1 },
    }
    const ops = [
      { kind: 'addShape', id, shape },
      { kind: 'addShape', id, shape, position: 0 },
      { kind: 'deleteShape', id },
      { kind: 'move', id, dx: 3, dy: 0 },
      { kind: 'setStyle', id, fill },
    ]
    doc.applyOp(JSON.stringify(ops[Math.floor(next() * ops.length)])) // refusals are fine: nothing changes
  }
}

let reordered = 0

describe.skipIf(!loadCore)('restoring with the real core', () => {
  it('turns a drawing back into a version a few edits away, drawing order included', async () => {
    const vec = await (await loadCore()).default()
    let replaced = 0
    for (let seed = 1; seed <= 200; seed++) {
      const next = random(seed)
      const current = new vec.Document()
      edit(current, next, 20)
      const target = new vec.Document()
      target.loadJson(current.shapesJson())
      edit(target, next, 4)
      const ops = restoreOps(JSON.parse(current.shapesJson()).shapes, JSON.parse(target.shapesJson()).shapes)
      for (const op of ops) {
        expect(
          JSON.parse(current.applyOp(JSON.stringify(op))).error,
          `seed ${seed}: ${JSON.stringify(op)}`,
        ).toBeUndefined()
      }
      expect(current.shapesJson(), `seed ${seed}`).toBe(target.shapesJson())
      // Restoring again needs nothing.
      expect(restoreOps(JSON.parse(current.shapesJson()).shapes, JSON.parse(target.shapesJson()).shapes)).toEqual([])
      replaced += ops.filter((op) => (op as { kind: string }).kind === 'addShape').length
      current.delete()
      target.delete()
    }
    expect(replaced).toBeGreaterThan(100) // the runs really exercised adding back
    expect(reordered).toBeGreaterThan(100) // and putting unchanged shapes back in order
  })
})
