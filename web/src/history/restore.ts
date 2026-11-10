// Turning the drawing back into an earlier version, with ordinary edits: every user sees it happen, the
// server checks each one, and it can be undone like anything else.

export interface ShapeEntry {
  id: string
  shape: { style?: object } & Record<string, unknown>
}

const same = (a: unknown, b: unknown) => JSON.stringify(a) === JSON.stringify(b)
// A shape without its style. JSON leaves out undefined fields.
const geometry = (shape: ShapeEntry['shape']) => ({ ...shape, style: undefined })

// The edits that make `current` into `target`, both bottom to top as the core's shapesJson lists them, and
// written by the same core, so equal shapes are equal JSON. A shape that's unchanged, and still in the same
// order relative to the others kept, stays; one whose style alone differs gets a setStyle; any other shape
// is deleted. Then every target shape not kept is added at its place in the drawing order.
export function restoreOps(current: ShapeEntry[], target: ShapeEntry[]): object[] {
  const wanted = new Map(target.map(({ id, shape }, position) => [id, { shape, position }]))
  const ops: object[] = []
  const kept = new Set<string>()
  let lastKept = -1 // the target position of the last shape kept: those kept must stay in order
  for (const { id, shape } of current) {
    const target = wanted.get(id)
    if (target && target.position > lastKept && same(geometry(shape), geometry(target.shape))) {
      kept.add(id)
      lastKept = target.position
      if (!same(shape.style, target.shape.style)) ops.push({ kind: 'setStyle', id, ...target.shape.style })
    } else {
      ops.push({ kind: 'deleteShape', id })
    }
  }
  // Every shape below `position` is now in place, so each goes exactly where it was.
  target.forEach(({ id, shape }, position) => {
    if (!kept.has(id)) ops.push({ kind: 'addShape', id, shape, position })
  })
  return ops
}
