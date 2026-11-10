import type { SyncStatus } from '../sync/SyncClient'

export interface Connection {
  status: SyncStatus
  joinedOnce: boolean // whether this page has ever been live
  online: boolean // what the browser says about its network
  pending: number // edits the server hasn't confirmed
}

// What to tell the user about their connection, and how worried to look. Unsaved edits are counted only
// while not connected: when connected they're saved within moments, and a count would flicker with each.
export function describeConnection({ status, joinedOnce, online, pending }: Connection) {
  if (online && status === 'live') return { label: 'Connected', tone: 'good' }
  const state = !online ? 'Offline' : joinedOnce ? 'Reconnecting…' : 'Connecting…'
  const waiting = pending === 1 ? '1 edit not saved yet' : `${pending} edits not saved yet`
  return { label: pending ? `${state} · ${waiting}` : state, tone: online ? 'waiting' : 'offline' }
}
