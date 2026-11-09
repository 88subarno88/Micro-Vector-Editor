import { describe, expect, it } from 'vitest'
import { describeRejection } from './rejections'

describe('describeRejection', () => {
  it('explains each reason the server gives', () => {
    expect(describeRejection('shape_not_found', '')).toBe("That edit wasn't saved: someone else deleted that shape first.")
    expect(describeRejection('duplicate_id', '')).toBe("That edit wasn't saved: a shape with the same ID was added at the same moment.")
    expect(describeRejection('forbidden', '')).toBe("That edit wasn't saved: you can only view this document.")
    expect(describeRejection('rate_limited', '')).toBe("That edit wasn't saved: too many edits too quickly. Try again.")
    expect(describeRejection('unavailable', '')).toBe("That edit wasn't saved: the server had a problem. Try again.")
    expect(describeRejection('not_found', '')).toBe("That edit wasn't saved: this document no longer exists.")
  })

  it("falls back to the server's own words for a reason it doesn't know", () => {
    expect(describeRejection('something_new', 'The moon is in the way')).toBe("That edit wasn't saved: The moon is in the way")
    expect(describeRejection('something_new', '')).toBe("That edit wasn't saved.")
  })
})
