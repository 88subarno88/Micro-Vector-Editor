import { describe, expect, it } from 'vitest'
import { describeConnection, type Connection } from './connection'

const live: Connection = { status: 'live', joinedOnce: true, online: true, pending: 0 }

describe('describeConnection', () => {
  it('says connected when live, without counting edits on their way', () => {
    expect(describeConnection(live)).toEqual({ label: 'Connected', tone: 'good' })
    expect(describeConnection({ ...live, pending: 2 })).toEqual({ label: 'Connected', tone: 'good' })
  })

  it('says connecting the first time, and reconnecting once it has been live', () => {
    expect(describeConnection({ ...live, status: 'connecting', joinedOnce: false }).label).toBe('Connecting…')
    expect(describeConnection({ ...live, status: 'joining', joinedOnce: false }).label).toBe('Connecting…')
    expect(describeConnection({ ...live, status: 'closed' })).toEqual({ label: 'Reconnecting…', tone: 'waiting' })
  })

  it('says offline when the browser has no network, whatever the socket thinks', () => {
    expect(describeConnection({ ...live, online: false })).toEqual({ label: 'Offline', tone: 'offline' })
  })

  it('counts the edits waiting while not connected', () => {
    expect(describeConnection({ ...live, status: 'closed', pending: 1 }).label).toBe(
      'Reconnecting… · 1 edit not saved yet',
    )
    expect(describeConnection({ ...live, online: false, pending: 3 }).label).toBe('Offline · 3 edits not saved yet')
  })
})
