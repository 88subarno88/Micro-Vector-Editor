// The picture beside the login form: a drawing in progress, with two other people's pointers moving over
// it, which is the app in one glance. Purely decorative.
export default function AuthArt() {
  return (
    <div className="auth-art" aria-hidden="true">
      <svg viewBox="0 0 420 300" className="auth-art-paper">
        <rect x="0" y="0" width="420" height="300" rx="16" fill="#ffffff" />
        <g stroke="#e8eaf2" strokeWidth="1">
          {Array.from({ length: 13 }, (_, i) => (
            <line key={`v${i}`} x1={i * 35} y1="0" x2={i * 35} y2="300" />
          ))}
          {Array.from({ length: 9 }, (_, i) => (
            <line key={`h${i}`} x1="0" y1={i * 37.5} x2="420" y2={i * 37.5} />
          ))}
        </g>
        <rect x="46" y="58" width="128" height="88" rx="14" fill="#e0e7ff" stroke="#6366f1" strokeWidth="2.5" />
        <circle cx="300" cy="96" r="46" fill="#fce7f3" stroke="#ec4899" strokeWidth="2.5" />
        <path d="M262 236 l26 -45 h52 l26 45 l-26 45 h-52 z" fill="#ecfdf5" stroke="#10b981" strokeWidth="2.5" />
        <g className="auth-art-curve">
          <path
            d="M60 230 C110 150 170 270 230 196"
            fill="none"
            stroke="#8b5cf6"
            strokeWidth="3"
            strokeLinecap="round"
          />
          <path d="M60 230 L110 150 M230 196 L170 270" stroke="#8b5cf6" strokeOpacity="0.45" strokeWidth="1.5" />
          <circle cx="110" cy="150" r="5" fill="#fff" stroke="#8b5cf6" strokeWidth="2" />
          <circle cx="170" cy="270" r="5" fill="#fff" stroke="#8b5cf6" strokeWidth="2" />
          <rect x="54" y="224" width="12" height="12" rx="2" fill="#8b5cf6" />
          <rect x="224" y="190" width="12" height="12" rx="2" fill="#8b5cf6" />
        </g>
        <rect x="40" y="52" width="140" height="100" rx="18" fill="none" stroke="#6366f1" strokeDasharray="5 4" />
      </svg>
      <div className="auth-art-pointer one">
        <svg width="18" height="24" viewBox="0 0 14 20">
          <path d="M1 1 L1 16 L5 12 L8 19 L10 18 L7 11 L13 11 Z" fill="#6366f1" stroke="#fff" />
        </svg>
        <span style={{ background: '#6366f1' }}>Ada</span>
      </div>
      <div className="auth-art-pointer two">
        <svg width="18" height="24" viewBox="0 0 14 20">
          <path d="M1 1 L1 16 L5 12 L8 19 L10 18 L7 11 L13 11 Z" fill="#ec4899" stroke="#fff" />
        </svg>
        <span style={{ background: '#ec4899' }}>Ravi</span>
      </div>
    </div>
  )
}
