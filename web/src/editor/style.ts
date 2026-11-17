// Turning shape styles into what form controls accept, and back.

// A few colours one click away, beside the full picker: ink and five clear hues for outlines, and their
// light tints, plus white, for fills.
export const strokePresets = [
  { name: 'Ink', color: '#1e1e2e' },
  { name: 'Red', color: '#e03131' },
  { name: 'Orange', color: '#f08c00' },
  { name: 'Green', color: '#2f9e44' },
  { name: 'Blue', color: '#1971c2' },
  { name: 'Violet', color: '#9c36b5' },
]
export const fillPresets = [
  { name: 'White', color: '#ffffff' },
  { name: 'Pink', color: '#ffc9c9' },
  { name: 'Yellow', color: '#ffec99' },
  { name: 'Mint', color: '#b2f2bb' },
  { name: 'Sky', color: '#a5d8ff' },
  { name: 'Lilac', color: '#eebefa' },
]

// The widest stroke the panel offers.
export const maxStrokeWidth = 50

// A colour as #rrggbb, which is all a colour picker understands. Shapes may hold any CSS colour ("red",
// "#f00", "rgb(...)"); the browser's own canvas parses those. `fallback` if it can't be parsed here.
export function toHexColor(color: string, fallback: string): string {
  if (/^#[0-9a-f]{6}$/i.test(color)) return color.toLowerCase()
  const short = /^#([0-9a-f])([0-9a-f])([0-9a-f])$/i.exec(color)
  if (short) return `#${short[1]}${short[1]}${short[2]}${short[2]}${short[3]}${short[3]}`.toLowerCase()

  const context = typeof document === 'undefined' ? null : document.createElement('canvas').getContext('2d')
  if (!context) return fallback
  context.fillStyle = fallback
  context.fillStyle = color // ignored if it isn't a colour, leaving the fallback
  return /^#[0-9a-f]{6}$/i.test(context.fillStyle) ? context.fillStyle : fallback
}

// A stroke width typed into the panel, or null if it isn't one: a number from 0 to maxStrokeWidth.
export function parseStrokeWidth(text: string): number | null {
  if (text.trim() === '') return null
  const width = Number(text)
  return Number.isFinite(width) && width >= 0 && width <= maxStrokeWidth ? width : null
}
