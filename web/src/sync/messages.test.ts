import { describe, expect, it } from 'vitest'
import { parseServerMessage } from './messages'

const parse = (message: unknown) => parseServerMessage(JSON.stringify(message))
const op = { kind: 'move', id: 's_1', dx: 1, dy: 2 }

describe('parseServerMessage', () => {
  it('reads every message the server sends', () => {
    expect(parse({ type: 'ready', role: 'editor' })).toEqual({ type: 'ready', role: 'editor' })
    expect(parse({ type: 'joined', headSeq: 2, ops: [{ seq: 2, userId: 'u', op }] })).toEqual({
      type: 'joined', headSeq: 2, ops: [{ seq: 2, userId: 'u', op }],
    })
    expect(parse({ type: 'joined', headSeq: 900, snapshot: { version: 1, shapes: [] } })).toEqual({
      type: 'joined', headSeq: 900, snapshot: { version: 1, shapes: [] },
    })
    expect(parse({ type: 'op', seq: 3, userId: 'u', op })).toEqual({ type: 'op', seq: 3, userId: 'u', op })
    expect(parse({ type: 'ack', clientOpId: 'c1', seq: 4 })).toEqual({ type: 'ack', clientOpId: 'c1', seq: 4 })
    expect(parse({ type: 'reject', clientOpId: 'c1', reason: 'forbidden', message: 'no' })).toEqual({
      type: 'reject', clientOpId: 'c1', reason: 'forbidden', message: 'no',
    })
    expect(parse({ type: 'presence', users: [{ id: 'u', name: 'ada', cursor: null }, { id: 'v', name: 'bo', cursor: { x: 1, y: 2 } }] }))
      .toMatchObject({ type: 'presence', users: [{ cursor: null }, { cursor: { x: 1, y: 2 } }] })
    expect(parse({ type: 'pong' })).toEqual({ type: 'pong' })
    expect(parse({ type: 'error', code: 'busy', message: 'try again' })).toEqual({ type: 'error', code: 'busy', message: 'try again' })
  })

  it('keeps only the fields it knows', () => {
    expect(parse({ type: 'ack', clientOpId: 'c1', seq: 4, extra: true })).toEqual({ type: 'ack', clientOpId: 'c1', seq: 4 })
  })

  it('ignores anything malformed or unknown', () => {
    const bad: unknown[] = [
      'not json', [], null, {}, { type: 'mystery' },
      { type: 'ready', role: 'admin' },
      { type: 'joined', headSeq: -1, ops: [] },
      { type: 'joined', headSeq: 1 },
      { type: 'joined', headSeq: 1, ops: [{ seq: 1, userId: 'u', op: 'not an object' }] },
      { type: 'op', seq: 1.5, userId: 'u', op },
      { type: 'ack', clientOpId: 7, seq: 1 },
      { type: 'presence', users: [{ id: 'u', name: 'ada', cursor: { x: 'left' } }] },
      { type: 'error' },
    ]
    for (const message of bad) {
      expect(typeof message === 'string' ? parseServerMessage(message) : parse(message)).toBeNull()
    }
  })
})
