// The messages on a document's WebSocket, /ws/docs/{id}, as the server defines them in
// server/api/collab/DocSocket.h. Ops themselves are the core's JSON, passed through untouched.

export type Role = 'owner' | 'editor' | 'viewer'

// An op someone committed, with its place in the document's history.
export interface CommittedOp {
  seq: number
  userId: string
  clientOpId: string // as its sender chose it: a client that missed the ack recognises its own op by this
  op: object
}

export interface PresenceUser {
  id: string
  name: string
  cursor: { x: number; y: number } | null
}

export type ServerMessage =
  | { type: 'ready'; role: Role }
  // Every op after the client's lastSeq, or, if the server no longer has them all, the whole drawing.
  | { type: 'joined'; headSeq: number; ops: CommittedOp[] }
  | { type: 'joined'; headSeq: number; snapshot: object }
  | ({ type: 'op' } & CommittedOp)
  | { type: 'ack'; clientOpId: string; seq: number }
  | { type: 'reject'; clientOpId: string; reason: string; message: string }
  | { type: 'presence'; users: PresenceUser[] }
  | { type: 'pong' }
  | { type: 'error'; code: string; message: string }

export type ClientMessage =
  | { type: 'join'; lastSeq: number }
  | { type: 'ping' }
  | { type: 'op'; clientOpId: string; baseSeq: number; op: object }
  | { type: 'cursor'; x: number; y: number }

// Why the server closed the connection, beyond the standard codes.
export const closeCodes = {
  unauthorized: 4401, // a bad or expired token
  notFound: 4404, // no such document, or not a member
  idle: 4408, // sent nothing for 45 s
  loadFailed: 1011, // the drawing couldn't be loaded
} as const

const isObject = (value: unknown): value is Record<string, unknown> =>
  typeof value === 'object' && value !== null && !Array.isArray(value)
const isSeq = (value: unknown): value is number => Number.isInteger(value) && (value as number) >= 0
const isString = (value: unknown): value is string => typeof value === 'string'

function isCommittedOp(value: unknown): value is CommittedOp {
  return (
    isObject(value) && isSeq(value.seq) && isString(value.userId) && isString(value.clientOpId) && isObject(value.op)
  )
}

function isPresenceUser(value: unknown): value is PresenceUser {
  if (!isObject(value) || !isString(value.id) || !isString(value.name)) return false
  const { cursor } = value
  return cursor === null || (isObject(cursor) && typeof cursor.x === 'number' && typeof cursor.y === 'number')
}

// A message from the server, checked field by field; null for anything malformed or unknown, which the
// client ignores rather than trusting.
export function parseServerMessage(text: string): ServerMessage | null {
  let message: unknown
  try {
    message = JSON.parse(text)
  } catch {
    return null
  }
  if (!isObject(message)) return null
  const m = message
  switch (m.type) {
    case 'ready':
      return m.role === 'owner' || m.role === 'editor' || m.role === 'viewer' ? { type: 'ready', role: m.role } : null
    case 'joined':
      if (!isSeq(m.headSeq)) return null
      if (Array.isArray(m.ops) && m.ops.every(isCommittedOp)) return { type: 'joined', headSeq: m.headSeq, ops: m.ops }
      if (isObject(m.snapshot)) return { type: 'joined', headSeq: m.headSeq, snapshot: m.snapshot }
      return null
    case 'op':
      return isCommittedOp(m) ? { type: 'op', seq: m.seq, userId: m.userId, clientOpId: m.clientOpId, op: m.op } : null
    case 'ack':
      return isString(m.clientOpId) && isSeq(m.seq) ? { type: 'ack', clientOpId: m.clientOpId, seq: m.seq } : null
    case 'reject':
      return isString(m.clientOpId) && isString(m.reason)
        ? { type: 'reject', clientOpId: m.clientOpId, reason: m.reason, message: isString(m.message) ? m.message : '' }
        : null
    case 'presence':
      return Array.isArray(m.users) && m.users.every(isPresenceUser) ? { type: 'presence', users: m.users } : null
    case 'pong':
      return { type: 'pong' }
    case 'error':
      return isString(m.code) ? { type: 'error', code: m.code, message: isString(m.message) ? m.message : '' } : null
  }
  return null
}
