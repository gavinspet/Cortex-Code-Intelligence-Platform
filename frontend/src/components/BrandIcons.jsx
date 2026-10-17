import { useId } from 'react'
import { cn } from './ui.jsx'

// Simplified brand marks drawn inline (24 x 24 viewBox). Each is shown beside, or labelled with, its written name.
export const BRAND_ICONS = {
  email: (
    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round">
      <rect x="3" y="5" width="18" height="14" rx="2.5" />
      <path d="m4 7 8 6 8-6" />
    </svg>
  ),
  linkedin: (
    <svg viewBox="0 0 24 24" fill="currentColor">
      <path d="M20.45 20.45h-3.55v-5.57c0-1.33-.03-3.04-1.85-3.04-1.86 0-2.14 1.45-2.14 2.94v5.67H9.35V9h3.41v1.56h.05c.48-.9 1.64-1.85 3.37-1.85 3.6 0 4.27 2.37 4.27 5.46v6.28zM5.34 7.43a2.06 2.06 0 1 1 0-4.13 2.06 2.06 0 0 1 0 4.13zM7.12 20.45H3.56V9h3.56v11.45z" />
    </svg>
  ),
  github: (
    <svg viewBox="0 0 24 24" fill="currentColor">
      <path d="M12 .3a12 12 0 0 0-3.8 23.38c.6.12.82-.26.82-.57v-2.04c-3.34.72-4.04-1.61-4.04-1.61-.55-1.39-1.34-1.76-1.34-1.76-1.09-.74.08-.73.08-.73 1.2.09 1.84 1.24 1.84 1.24 1.07 1.83 2.81 1.3 3.5 1 .1-.78.42-1.3.76-1.6-2.67-.3-5.47-1.33-5.47-5.93 0-1.31.47-2.38 1.24-3.22-.14-.3-.54-1.52.1-3.18 0 0 1-.32 3.3 1.23a11.5 11.5 0 0 1 6 0c2.28-1.55 3.29-1.23 3.29-1.23.64 1.66.24 2.88.12 3.18a4.65 4.65 0 0 1 1.23 3.22c0 4.61-2.8 5.63-5.48 5.92.42.36.81 1.1.81 2.22v3.29c0 .32.21.7.82.58A12 12 0 0 0 12 .3z" />
    </svg>
  ),
  gitlab: (
    <svg viewBox="0 0 24 24" fill="currentColor">
      <path d="M12 21.5 2.4 14.3a1 1 0 0 1-.36-1.1l1.2-3.7L5.6 2.3a.5.5 0 0 1 .95 0l2.36 7.2h6.18l2.36-7.2a.5.5 0 0 1 .95 0l2.36 7.2 1.2 3.7a1 1 0 0 1-.36 1.1z" />
    </svg>
  ),
  instagram: (
    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
      <rect x="3" y="3" width="18" height="18" rx="5" />
      <circle cx="12" cy="12" r="4" />
      <path d="M17.5 6.5h.01" />
    </svg>
  ),
}

export function BrandLogo({ id, className }) {
  return <span className={cn('brand-logo', `brand-logo-${id}`, className)} aria-hidden="true">{BRAND_ICONS[id]}</span>
}

// Cortex mark: a "C" whose ends are connected nodes, on a blue gradient tile.
export function CortexMark({ size = 28, className }) {
  const gradientId = `cortex-mark-${useId().replace(/:/g, '')}`
  return (
    <svg className={cn('cortex-mark', className)} viewBox="0 0 32 32" width={size} height={size} aria-hidden="true" focusable="false">
      <defs>
        <linearGradient id={gradientId} x1="0" y1="0" x2="1" y2="1">
          <stop offset="0" stopColor="#3b82f6" />
          <stop offset="1" stopColor="#4f46e5" />
        </linearGradient>
      </defs>
      <rect width="32" height="32" rx="9" fill={`url(#${gradientId})`} />
      <path d="M21.5 10.8a7.4 7.4 0 1 0 0 10.4" fill="none" stroke="#ffffff" strokeWidth="2.6" strokeLinecap="round" />
      <path d="M14.5 16h5" stroke="#bfdbfe" strokeWidth="2" strokeLinecap="round" />
      <circle cx="21.5" cy="10.8" r="2.2" fill="#ffffff" />
      <circle cx="21.5" cy="21.2" r="2.2" fill="#ffffff" />
      <circle cx="21.5" cy="16" r="1.8" fill="#bfdbfe" />
    </svg>
  )
}
