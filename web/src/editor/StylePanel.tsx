import { useEffect, useRef, useState, type KeyboardEvent } from 'react'
import type { Style } from './paint'
import { maxStrokeWidth, parseStrokeWidth, toHexColor } from './style'

export type StyleChange = Partial<Style>

// Stroke colour, fill colour and stroke width for the selected shape. Changes show at once (onPreview) and
// become one edit when finished (onCommit): when a colour picker closes, a fill is turned on or off, or the
// width is entered. A colour picker reports every step while it's dragged; committing each would send a
// stream of edits. Remount it (with a key) when the selection or its committed style changes.
export default function StylePanel({ style, canFill, onPreview, onCommit, onCancel }: {
  style: Style
  canFill: boolean // false for open shapes, like lines, which are never filled
  onPreview(change: StyleChange): void
  onCommit(change: StyleChange): void
  onCancel(): void
}) {
  const [width, setWidth] = useState(String(style.strokeWidth))
  const [fill, setFill] = useState(toHexColor(style.fill, '#ffffff'))
  const stroke = useRef<HTMLInputElement>(null)
  const fillPicker = useRef<HTMLInputElement>(null)

  // A colour picker's "change" event fires once, when it closes; React's onChange fires on every step.
  useEffect(() => {
    const pickers: [HTMLInputElement | null, keyof Style][] = [[stroke.current, 'stroke'], [fillPicker.current, 'fill']]
    const listeners = pickers.map(([picker, key]) => {
      const listener = (event: Event) => onCommit({ [key]: (event.target as HTMLInputElement).value })
      picker?.addEventListener('change', listener)
      return () => picker?.removeEventListener('change', listener)
    })
    return () => listeners.forEach((remove) => remove())
  }, [onCommit])

  function commitWidth() {
    const parsed = parseStrokeWidth(width)
    if (parsed === null || parsed === style.strokeWidth) {
      setWidth(String(style.strokeWidth))
      onCancel()
    } else {
      onCommit({ strokeWidth: parsed })
    }
  }

  function onWidthKey(event: KeyboardEvent<HTMLInputElement>) {
    if (event.key === 'Enter') commitWidth()
    if (event.key === 'Escape') {
      setWidth(String(style.strokeWidth))
      onCancel()
    }
  }

  const filled = style.fill !== 'none'
  return (
    <div className="style-panel" role="group" aria-label="Style">
      <label>
        Stroke
        <input ref={stroke} type="color" defaultValue={toHexColor(style.stroke, '#000000')}
          onChange={(e) => onPreview({ stroke: e.target.value })} />
      </label>
      {canFill && (
        <>
          <label>
            Fill
            <input ref={fillPicker} type="color" value={fill} disabled={!filled}
              onChange={(e) => { setFill(e.target.value); onPreview({ fill: e.target.value }) }} />
          </label>
          <label className="check">
            <input type="checkbox" checked={!filled} onChange={(e) => onCommit({ fill: e.target.checked ? 'none' : fill })} />
            No fill
          </label>
        </>
      )}
      <label>
        Width
        <input type="number" min={0} max={maxStrokeWidth} step={0.5} value={width}
          onChange={(e) => {
            setWidth(e.target.value)
            const parsed = parseStrokeWidth(e.target.value)
            if (parsed !== null) onPreview({ strokeWidth: parsed })
          }}
          onKeyDown={onWidthKey} onBlur={commitWidth} />
      </label>
    </div>
  )
}
