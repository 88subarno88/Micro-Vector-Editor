// What to tell the user when the server refuses one of their edits. The edit has already been taken back out
// of their drawing; these say why.

const reasons: Record<string, string> = {
  shape_not_found: 'someone else deleted that shape first.',
  duplicate_id: 'a shape with the same ID was added at the same moment.',
  invalid_value: 'it would take the shape beyond the drawing.',
  invalid_shape: "that shape can't be drawn.",
  bad_op: "the edit couldn't be read.",
  forbidden: 'you can only view this document.',
  not_found: 'this document no longer exists.',
  rate_limited: 'too many edits too quickly. Try again.',
  unavailable: 'the server had a problem. Try again.',
}

// `reason` and `message` as the server sent them in its reject.
export function describeRejection(reason: string, message: string): string {
  const why = reasons[reason] ?? message
  return why ? `That edit wasn't saved: ${why}` : "That edit wasn't saved."
}
