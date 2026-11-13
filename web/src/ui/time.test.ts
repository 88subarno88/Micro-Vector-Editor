import { describe, expect, it } from 'vitest'
import { timeAgo } from './time'

const now = Date.parse('2026-11-14T12:00:00Z')
const ago = (seconds: number) => new Date(now - seconds * 1000).toISOString()

describe('timeAgo', () => {
  it('says how long ago, in the largest unit that fits', () => {
    expect(timeAgo(ago(10), now)).toBe('just now')
    expect(timeAgo(ago(5 * 60), now)).toMatch(/5 minutes ago/)
    expect(timeAgo(ago(3 * 3600), now)).toMatch(/3 hours ago/)
    expect(timeAgo(ago(26 * 3600), now)).toMatch(/yesterday/)
  })

  it('gives the date after a week, with the year only when it differs', () => {
    expect(timeAgo('2026-10-01T12:00:00Z', now)).not.toMatch(/2026/)
    expect(timeAgo('2025-10-01T12:00:00Z', now)).toMatch(/2025/)
  })

  it("says nothing for a time it can't read", () => {
    expect(timeAgo('x', now)).toBe('')
  })
})
