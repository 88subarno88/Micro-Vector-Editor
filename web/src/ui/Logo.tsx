import { useId } from 'react'

// The mark: a vector path with its editing handles, on the brand gradient. Beside it, the name.
export function LogoMark({ size = 32 }: { size?: number }) {
  const gradient = useId()
  return (
    <svg width={size} height={size} viewBox="0 0 32 32" aria-hidden="true" focusable="false">
      <defs>
        <linearGradient id={gradient} x1="0" y1="0" x2="1" y2="1">
          <stop offset="0" stopColor="#6366f1" />
          <stop offset="0.55" stopColor="#8b5cf6" />
          <stop offset="1" stopColor="#ec4899" />
        </linearGradient>
      </defs>
      <rect width="32" height="32" rx="8" fill={`url(#${gradient})`} />
      <path d="M8 22c2-9 6-12 8-12s4 3 8 12" fill="none" stroke="#fff" strokeWidth="2.2" strokeLinecap="round" />
      <path d="M16 10 9.5 7.5M16 10l6.5-2.5" stroke="#fff" strokeOpacity="0.6" strokeWidth="1.2" />
      <circle cx="9.5" cy="7.5" r="1.6" fill="#fff" />
      <circle cx="22.5" cy="7.5" r="1.6" fill="#fff" />
      <rect x="14.4" y="8.4" width="3.2" height="3.2" rx="0.6" fill="#fff" />
    </svg>
  )
}

export default function Logo({ size = 32 }: { size?: number }) {
  return (
    <span className="logo">
      <LogoMark size={size} />
      <span className="logo-name">
        Micro<span>Vector</span>
      </span>
    </span>
  )
}
