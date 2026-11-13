import type { SVGProps } from 'react'

// The app's icons: simple strokes on a 24-unit grid, drawn in the current text colour. Decorative: whatever
// they sit in carries the name.
function Icon({ children, ...props }: SVGProps<SVGSVGElement>) {
  return (
    <svg
      viewBox="0 0 24 24"
      width="20"
      height="20"
      fill="none"
      stroke="currentColor"
      strokeWidth="1.8"
      strokeLinecap="round"
      strokeLinejoin="round"
      aria-hidden="true"
      focusable="false"
      {...props}
    >
      {children}
    </svg>
  )
}

type P = SVGProps<SVGSVGElement>

export const SelectIcon = (p: P) => (
  <Icon {...p}>
    <path d="M5 3.5 18.5 10l-6 1.8-2.3 6.2z" />
  </Icon>
)
export const RectIcon = (p: P) => (
  <Icon {...p}>
    <rect x="4" y="6" width="16" height="12" rx="0.5" />
  </Icon>
)
export const RoundedRectIcon = (p: P) => (
  <Icon {...p}>
    <rect x="4" y="6" width="16" height="12" rx="4" />
  </Icon>
)
export const CircleIcon = (p: P) => (
  <Icon {...p}>
    <circle cx="12" cy="12" r="7.5" />
  </Icon>
)
export const LineIcon = (p: P) => (
  <Icon {...p}>
    <path d="M5 19 19 5" />
  </Icon>
)
export const HexagonIcon = (p: P) => (
  <Icon {...p}>
    <path d="M12 3.5 19.4 7.75v8.5L12 20.5 4.6 16.25v-8.5z" />
  </Icon>
)
export const PenIcon = (p: P) => (
  <Icon {...p}>
    <path d="M4 17c3-6 5 2 8-4s5-6 8-6" />
  </Icon>
)
export const TextIcon = (p: P) => (
  <Icon {...p}>
    <path d="M5 6.5V5h14v1.5M12 5v14M9 19h6" />
  </Icon>
)
export const UndoIcon = (p: P) => (
  <Icon {...p}>
    <path d="M9 14 4 9l5-5" />
    <path d="M4 9h10.5a5.5 5.5 0 0 1 0 11H11" />
  </Icon>
)
export const RedoIcon = (p: P) => (
  <Icon {...p}>
    <path d="m15 14 5-5-5-5" />
    <path d="M20 9H9.5a5.5 5.5 0 0 0 0 11H13" />
  </Icon>
)
export const TrashIcon = (p: P) => (
  <Icon {...p}>
    <path d="M4 7h16M10 11v6M14 11v6M6 7l1 12a2 2 0 0 0 2 2h6a2 2 0 0 0 2-2l1-12M9 7V4h6v3" />
  </Icon>
)
export const MinusIcon = (p: P) => (
  <Icon {...p}>
    <path d="M5 12h14" />
  </Icon>
)
export const PlusIcon = (p: P) => (
  <Icon {...p}>
    <path d="M12 5v14M5 12h14" />
  </Icon>
)
export const DownloadIcon = (p: P) => (
  <Icon {...p}>
    <path d="M12 4v11M7 10l5 5 5-5M5 20h14" />
  </Icon>
)
export const UploadIcon = (p: P) => (
  <Icon {...p}>
    <path d="M12 16V5M7 10l5-5 5 5M5 20h14" />
  </Icon>
)
export const HistoryIcon = (p: P) => (
  <Icon {...p}>
    <path d="M3.5 12a8.5 8.5 0 1 0 2.5-6" />
    <path d="M3 4v4h4M12 8v4l3 2" />
  </Icon>
)
export const BackIcon = (p: P) => (
  <Icon {...p}>
    <path d="M15 5 8 12l7 7" />
  </Icon>
)
export const LogOutIcon = (p: P) => (
  <Icon {...p}>
    <path d="M14 4h4a2 2 0 0 1 2 2v12a2 2 0 0 1-2 2h-4M10 16l-4-4 4-4M6 12h10" />
  </Icon>
)
export const SparkleIcon = (p: P) => (
  <Icon {...p}>
    <path d="M12 3.5 13.8 9l5.7 1.8-5.7 1.9L12 18.2l-1.8-5.5-5.7-1.9L10.2 9z" />
  </Icon>
)
export const UsersIcon = (p: P) => (
  <Icon {...p}>
    <circle cx="9" cy="8.5" r="3.5" />
    <path d="M3 19.5c.8-3.2 3.2-5 6-5s5.2 1.8 6 5" />
    <path d="M16 5.2a3.4 3.4 0 0 1 0 6.6M18 14.6c1.4.8 2.5 2.4 3 4.9" />
  </Icon>
)
export const WifiOffIcon = (p: P) => (
  <Icon {...p}>
    <path d="M3 3l18 18M8.5 16.5a5 5 0 0 1 7 0M5 13a10 10 0 0 1 5-2.6M14.5 10.6A10 10 0 0 1 19 13M2 9.5a15 15 0 0 1 4.3-2.7M10.7 5.1A15 15 0 0 1 22 9.5" />
    <circle cx="12" cy="20" r="0.6" fill="currentColor" />
  </Icon>
)
export const LayersIcon = (p: P) => (
  <Icon {...p}>
    <path d="m12 3 9 5-9 5-9-5z" />
    <path d="m3 13 9 5 9-5" />
  </Icon>
)
export const FileIcon = (p: P) => (
  <Icon {...p}>
    <path d="M14 3H7a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h10a2 2 0 0 0 2-2V8z" />
    <path d="M14 3v5h5" />
  </Icon>
)
export const EyeIcon = (p: P) => (
  <Icon {...p}>
    <path d="M2.5 12S6 5.5 12 5.5 21.5 12 21.5 12 18 18.5 12 18.5 2.5 12 2.5 12z" />
    <circle cx="12" cy="12" r="3" />
  </Icon>
)
export const PencilIcon = (p: P) => (
  <Icon {...p}>
    <path d="M4 20h4L19 9a2.8 2.8 0 0 0-4-4L4 16z" />
  </Icon>
)
export const RestoreIcon = (p: P) => (
  <Icon {...p}>
    <path d="M20.5 12a8.5 8.5 0 1 1-2.5-6" />
    <path d="M21 4v4h-4" />
  </Icon>
)
