import { useEffect, useId, useRef, useState, type KeyboardEvent } from 'react'
import type { Style } from './paint'
import { fillPresets, maxStrokeWidth, parseStrokeWidth, strokePresets, toHexColor } from './style'

export type StyleChange = Partial<Style>

function Swatches({
  label,
  presets,
  current,
  onPick,
}: {
  label: string
  presets: { name: string; color: string }[]
  current: string | null
  onPick(color: string): void
}) {
  return (
    <div className="swatches" role="group" aria-label={label}>
      {presets.map(({ name, color }) => (
        <button
          key={color}
          type="button"
          className="swatch"
          aria-label={name}
          aria-pressed={current === color}
          style={{ background: color }}
          onClick={() => onPick(color)}
        >
          <span className="tip" aria-hidden="true">
            {name}
          </span>
        </button>
      ))}
    </div>
  )
}

// Stroke colour, fill colour and stroke width for the selected shape. Changes show at once (onPreview) and
// become one edit when finished (onCommit): when a colour picker closes, a preset or a fill is picked or
// turned off, or the width is entered. A colour picker reports every step while it's dragged; committing each
// would send a stream of edits. Remount it (with a key) when the selection or its committed style changes.
export default function StylePanel({
  style,
  canFill,
  onPreview,
  onCommit,
  onCancel,
}: {
  style: Style
  canFill: boolean // false for open shapes, like lines, which are never filled
  onPreview(change: StyleChange): void
  onCommit(change: StyleChange): void
  onCancel(): void
}) {
  const ids = { stroke: useId(), fill: useId(), width: useId() }
  const [width, setWidth] = useState(String(style.strokeWidth))
  const [strokeShown, setStrokeShown] = useState(toHexColor(style.stroke, '#000000'))
  const [fill, setFill] = useState(toHexColor(style.fill, '#ffffff'))
  const stroke = useRef<HTMLInputElement>(null)
  const fillPicker = useRef<HTMLInputElement>(null)

  // A colour picker's "change" event fires once, when it closes; React's onChange fires on every step.
  useEffect(() => {
    const pickers: [HTMLInputElement | null, keyof Style][] = [
      [stroke.current, 'stroke'],
      [fillPicker.current, 'fill'],
    ]
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
      <div className="style-row">
        <label htmlFor={ids.stroke}>Stroke</label>
        <div className="style-control">
          <input
            id={ids.stroke}
            ref={stroke}
            type="color"
            defaultValue={toHexColor(style.stroke, '#000000')}
            onChange={(e) => {
              setStrokeShown(e.target.value)
              onPreview({ stroke: e.target.value })
            }}
          />
          <code className="hex">{strokeShown}</code>
        </div>
      </div>
      <Swatches
        label="Outline colours"
        presets={strokePresets}
        current={toHexColor(style.stroke, '')}
        onPick={(color) => onCommit({ stroke: color })}
      />

      {canFill && (
        <>
          <div className="style-row">
            <label htmlFor={ids.fill}>Fill</label>
            <div className="style-control">
              <input
                id={ids.fill}
                ref={fillPicker}
                type="color"
                value={fill}
                disabled={!filled}
                onChange={(e) => {
                  setFill(e.target.value)
                  onPreview({ fill: e.target.value })
                }}
              />
              <code className="hex">{filled ? fill : 'none'}</code>
              <label className="check">
                <input
                  type="checkbox"
                  checked={!filled}
                  onChange={(e) => onCommit({ fill: e.target.checked ? 'none' : fill })}
                />
                No fill
              </label>
            </div>
          </div>
          <Swatches
            label="Fill colours"
            presets={fillPresets}
            current={filled ? toHexColor(style.fill, '') : null}
            onPick={(color) => onCommit({ fill: color })}
          />
        </>
      )}

      <div className="style-row">
        <label htmlFor={ids.width}>Width</label>
        <div className="style-control">
          <input
            id={ids.width}
            type="number"
            min={0}
            max={maxStrokeWidth}
            step={0.5}
            value={width}
            onChange={(e) => {
              setWidth(e.target.value)
              const parsed = parseStrokeWidth(e.target.value)
              if (parsed !== null) onPreview({ strokeWidth: parsed })
            }}
            onKeyDown={onWidthKey}
            onBlur={commitWidth}
          />
          <span className="unit">px</span>
        </div>
      </div>
    </div>
  )
}
