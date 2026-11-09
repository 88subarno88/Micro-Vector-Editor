// @vitest-environment jsdom
import { afterEach, describe, expect, it, vi } from 'vitest'
import { parseStrokeWidth, toHexColor } from './style'

afterEach(() => vi.restoreAllMocks())

describe('toHexColor', () => {
  it('keeps #rrggbb, in lower case', () => {
    expect(toHexColor('#00FF00', '#000000')).toBe('#00ff00')
  })

  it('expands #rgb', () => {
    expect(toHexColor('#f0a', '#000000')).toBe('#ff00aa')
  })

  it("lets the browser's canvas parse any other colour, and falls back if it can't", () => {
    // jsdom has no canvas, so stand in for one: it understands "red" and ignores anything it can't parse.
    const context = {
      value: '#000000',
      get fillStyle() { return this.value },
      set fillStyle(color: string) {
        if (color === 'red') this.value = '#ff0000'
        else if (/^#[0-9a-f]{6}$/.test(color)) this.value = color
      },
    }
    vi.spyOn(HTMLCanvasElement.prototype, 'getContext').mockReturnValue(context as unknown as CanvasRenderingContext2D)

    expect(toHexColor('red', '#123456')).toBe('#ff0000')
    expect(toHexColor('not a colour', '#123456')).toBe('#123456')
  })

  it('falls back where there is no canvas at all', () => {
    vi.spyOn(HTMLCanvasElement.prototype, 'getContext').mockReturnValue(null)
    expect(toHexColor('red', '#123456')).toBe('#123456')
  })
})

describe('parseStrokeWidth', () => {
  it('reads widths from 0 to 50', () => {
    expect(parseStrokeWidth('0')).toBe(0)
    expect(parseStrokeWidth('2.5')).toBe(2.5)
    expect(parseStrokeWidth('50')).toBe(50)
  })

  it('refuses anything else', () => {
    for (const text of ['', ' ', '-1', '51', 'abc', 'Infinity']) expect(parseStrokeWidth(text)).toBeNull()
  })
})
