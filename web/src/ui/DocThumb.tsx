// A small picture for a document's card. There's no stored preview of the drawing, so each document gets an
// abstract composition of shapes, chosen from its ID: the same every time, different between documents.

const palette = [
  ['#6366f1', '#e0e7ff'],
  ['#ec4899', '#fce7f3'],
  ['#10b981', '#d1fae5'],
  ['#f59e0b', '#fef3c7'],
  ['#8b5cf6', '#ede9fe'],
  ['#0ea5e9', '#e0f2fe'],
]

// A small seeded generator, so the picture follows from the ID.
function generator(id: string) {
  let seed = 0
  for (const char of id) seed = (Math.imul(seed, 31) + char.charCodeAt(0)) | 0
  return () => {
    seed = (seed + 0x6d2b79f5) | 0
    let t = Math.imul(seed ^ (seed >>> 15), 1 | seed)
    t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t
    return ((t ^ (t >>> 14)) >>> 0) / 4294967296
  }
}

export default function DocThumb({ id }: { id: string }) {
  const next = generator(id)
  const pick = <T,>(items: T[]) => items[Math.floor(next() * items.length)]
  const shapes = Array.from({ length: 3 }, (_, i) => {
    const [stroke, fill] = pick(palette)
    const x = 30 + i * 70 + next() * 30
    const y = 40 + next() * 70
    const size = 34 + next() * 26
    switch (Math.floor(next() * 4)) {
      case 0:
        return (
          <rect
            key={i}
            x={x - size / 2}
            y={y - size / 2}
            width={size * 1.3}
            height={size}
            rx={size * 0.2}
            fill={fill}
            stroke={stroke}
            strokeWidth="2.5"
          />
        )
      case 1:
        return <circle key={i} cx={x} cy={y} r={size / 2} fill={fill} stroke={stroke} strokeWidth="2.5" />
      case 2: {
        const r = size / 2
        const points = Array.from(
          { length: 6 },
          (_, k) => `${x + r * Math.cos((Math.PI / 3) * k)},${y + r * Math.sin((Math.PI / 3) * k)}`,
        )
        return <polygon key={i} points={points.join(' ')} fill={fill} stroke={stroke} strokeWidth="2.5" />
      }
      default:
        return (
          <path
            key={i}
            d={`M${x - size / 2} ${y + size / 3} C${x - size / 4} ${y - size / 2} ${x + size / 4} ${y + size / 2} ${x + size / 2} ${y - size / 3}`}
            fill="none"
            stroke={stroke}
            strokeWidth="3"
            strokeLinecap="round"
          />
        )
    }
  })
  return (
    <svg
      viewBox="0 0 260 150"
      className="doc-thumb-art"
      aria-hidden="true"
      focusable="false"
      preserveAspectRatio="xMidYMid slice"
    >
      <rect width="260" height="150" fill="#ffffff" />
      <g stroke="#eef0f6">
        {Array.from({ length: 9 }, (_, i) => (
          <line key={`v${i}`} x1={i * 32.5} y1="0" x2={i * 32.5} y2="150" />
        ))}
        {Array.from({ length: 5 }, (_, i) => (
          <line key={`h${i}`} x1="0" y1={i * 37.5} x2="260" y2={i * 37.5} />
        ))}
      </g>
      {shapes}
    </svg>
  )
}
