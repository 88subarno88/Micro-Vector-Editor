const relative = new Intl.RelativeTimeFormat(undefined, { numeric: 'auto' })
const shortDate = new Intl.DateTimeFormat(undefined, { month: 'short', day: 'numeric' })
const fullDate = new Intl.DateTimeFormat(undefined, { dateStyle: 'medium' })

// When something happened, as people say it: "just now", "5 minutes ago", "yesterday", then "Nov 14" within
// the year, and "Nov 14, 2025" before it. Empty for a time that can't be read.
export function timeAgo(iso: string, now = Date.now()): string {
  const then = new Date(iso)
  if (Number.isNaN(then.getTime())) return ''
  const seconds = Math.round((then.getTime() - now) / 1000)
  if (seconds > -45) return 'just now'
  const minutes = Math.round(seconds / 60)
  if (minutes > -60) return relative.format(minutes, 'minute')
  const hours = Math.round(minutes / 60)
  if (hours > -24) return relative.format(hours, 'hour')
  const days = Math.round(hours / 24)
  if (days > -7) return relative.format(days, 'day')
  return then.getFullYear() === new Date(now).getFullYear() ? shortDate.format(then) : fullDate.format(then)
}
