import { describe, expect, it } from 'vitest'
import { describeFailure } from './errors'

describe('describeFailure', () => {
  it('says how long to wait when rate limited', () => {
    const response = new Response(null, { status: 429, headers: { 'Retry-After': '42' } })
    expect(describeFailure({ error: { code: 'rate_limited', message: 'x' } }, response)).toBe(
      'Too many attempts. Try again in 42 seconds.',
    )
  })

  it('reports a server it cannot reach', () => {
    expect(describeFailure(undefined, undefined)).toMatch(/Can't reach the server/)
  })

  it("falls back to the server's own message for codes it doesn't know", () => {
    expect(
      describeFailure({ error: { code: 'odd', message: 'An odd thing.' } }, new Response(null, { status: 400 })),
    ).toBe('An odd thing.')
  })
})
