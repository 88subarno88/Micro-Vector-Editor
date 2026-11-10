import { describeConnection, type Connection } from './connection'

// The connection's state, as a dot and a few words.
export default function ConnectionStatus(connection: Connection) {
  const { label, tone } = describeConnection(connection)
  return (
    <p className={`connection ${tone}`} role="status">
      {label}
    </p>
  )
}
