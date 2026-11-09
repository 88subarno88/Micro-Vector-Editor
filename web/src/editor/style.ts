// Turning shape styles into what form controls accept, and back.

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
