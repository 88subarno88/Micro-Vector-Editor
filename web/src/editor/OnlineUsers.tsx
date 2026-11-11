import type { PresenceUser } from '../sync/messages'
import { colorFor } from './cursors'

// At most this many avatars; the rest are counted.
export const maxAvatars = 5

// Who has the document open: a round avatar per user, in their pointer's colour, however many tabs they
// have open. This user is marked as "you".
export default function OnlineUsers({ users, me }: { users: PresenceUser[]; me: string }) {
  const unique = users.filter((user, index) => users.findIndex((other) => other.id === user.id) === index)
  const shown = unique.slice(0, maxAvatars)
  const hidden = unique.length - shown.length
  return (
    <ul className="online" aria-label="Online now">
      {shown.map(({ id, name }) => {
        const label = id === me ? `${name} (you)` : name
        return (
          <li key={id} className="avatar" title={label} aria-label={label} style={{ background: colorFor(id) }}>
            {name.charAt(0).toUpperCase() || '?'}
          </li>
        )
      })}
      {hidden > 0 && (
        <li
          className="avatar more"
          title={unique
            .slice(maxAvatars)
            .map((user) => user.name)
            .join(', ')}
          aria-label={`${hidden} more`}
        >
          +{hidden}
        </li>
      )}
    </ul>
  )
}
