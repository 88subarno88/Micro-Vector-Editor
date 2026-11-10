import { describe, expect, it } from 'vitest'
import { History } from './history'

const add = { kind: 'addShape', id: 'a' }
const remove = { kind: 'deleteShape', id: 'a' }
const move = (dx: number, dy = 0) => ({ kind: 'move', id: 'a', dx, dy })

describe('History', () => {
  it('undoes the latest edit first', () => {
    const history = new History()
    history.record(add, remove)
    const moved = move(5)
    history.record(moved, move(-5))

    expect(history.takeUndo()).toEqual({ made: [moved], reverse: [move(-5)], group: undefined })
    expect(history.takeUndo()?.reverse).toEqual([remove])
    expect(history.canUndo).toBe(false)
  })

  it('joins the moves of one drag into one step, reversed by one move by their total', () => {
    const history = new History()
    const drag = {}
    const parts = [move(3, 1), move(4, 2), move(1, 0)]
    for (const part of parts) history.record(part, move(-part.dx, -part.dy), drag)
    history.record(move(9), move(-9), {}) // another drag

    expect(history.takeUndo()?.reverse).toEqual([move(-9)])
    const step = history.takeUndo()!
    expect(step.reverse).toEqual([move(-8, -3)])
    expect(step.made).toEqual(parts)
    expect(history.canUndo).toBe(false)
  })

  it('joins any edits of one group into one step, undone newest first', () => {
    const history = new History()
    const restore = {}
    history.record(remove, add, restore)
    history.record({ kind: 'setStyle', id: 'b', fill: 'red' }, { kind: 'setStyle', id: 'b', fill: 'none' }, restore)
    history.record(move(2), move(-2), restore) // a move after other edits isn't merged with anything

    expect(history.takeUndo()?.reverse).toEqual([move(-2), { kind: 'setStyle', id: 'b', fill: 'none' }, add])
    expect(history.canUndo).toBe(false)
  })

  it('keeps moves of different shapes in one group as separate ops', () => {
    const history = new History()
    const group = {}
    history.record(move(1), move(-1), group)
    history.record({ ...move(1), id: 'b' }, { ...move(-1), id: 'b' }, group)
    expect(history.takeUndo()?.reverse).toEqual([{ ...move(-1), id: 'b' }, move(-1)])
  })

  it('keeps redo until a new edit is made', () => {
    const history = new History()
    history.pushRedo({ made: [remove], reverse: [add] })
    expect(history.canRedo).toBe(true)
    history.record(move(1), move(-1))
    expect(history.canRedo).toBe(false)
  })

  it('forgets the step an op belongs to when the server refuses it, in either list', () => {
    const history = new History()
    const drag = {}
    const first = move(1)
    const refused = move(2)
    history.record(add, remove)
    history.record(first, move(-1), drag)
    history.record(refused, move(-2), drag)
    const undone = { kind: 'setStyle', id: 'a' }
    history.pushRedo({ made: [undone], reverse: [] })

    history.forget(refused) // the whole drag goes: its total no longer says where the shape was
    history.forget(undone)

    expect(history.canRedo).toBe(false)
    expect(history.takeUndo()?.reverse).toEqual([remove])
    expect(history.canUndo).toBe(false)
  })

  it('forgets nothing for an op it never recorded, even an equal one', () => {
    const history = new History()
    history.record(move(1), move(-1))
    history.forget(move(1))
    expect(history.canUndo).toBe(true)
  })
})
