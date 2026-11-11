// @vitest-environment jsdom
import '@testing-library/jest-dom/vitest'
import { cleanup, render, screen, within } from '@testing-library/react'
import { afterEach, describe, expect, it } from 'vitest'
import type { PresenceUser } from '../sync/messages'
import { colorFor } from './cursors'
import OnlineUsers from './OnlineUsers'

const user = (id: string, name: string): PresenceUser => ({ id, name, cursor: null })
const avatars = () => within(screen.getByRole('list', { name: 'Online now' })).queryAllByRole('listitem')

afterEach(cleanup)

describe('OnlineUsers', () => {
  it('shows each user once, by initial, in their colour, with this user marked', () => {
    render(
      <OnlineUsers
        me="u1"
        users={[user('u1', 'ada@example.com'), user('u2', 'bob@example.com'), user('u2', 'bob@example.com')]}
      />,
    )

    expect(avatars().map((a) => a.getAttribute('aria-label'))).toEqual(['ada@example.com (you)', 'bob@example.com'])
    expect(avatars().map((a) => a.textContent)).toEqual(['A', 'B'])
    expect(avatars()[1]).toHaveStyle({ background: colorFor('u2') })
  })

  it('counts the users beyond five instead of showing them, naming them on hover', () => {
    const many = ['a', 'b', 'c', 'd', 'e', 'f', 'g'].map((name, i) => user(`u${i}`, name))
    render(<OnlineUsers me="u0" users={many} />)

    expect(avatars()).toHaveLength(6)
    const more = screen.getByLabelText('2 more')
    expect(more).toHaveTextContent('+2')
    expect(more).toHaveAttribute('title', 'f, g')
  })

  it('shows nothing while nobody is known to be there', () => {
    render(<OnlineUsers me="u1" users={[]} />)
    expect(avatars()).toEqual([])
  })
})
