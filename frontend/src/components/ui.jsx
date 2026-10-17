export function cn(...parts) {
  return parts.filter(Boolean).join(' ')
}

export function getErrorMessage(message) {
  const text = (message || '').toLowerCase()
  if (!message) return 'The request could not be completed.'
  if (text.includes('busy')) return 'The queue is currently busy. The distributed worker pipeline rejected new work for the moment.'
  if (text.includes('timeout')) return 'The backend did not respond in time. The request may still be running on the server.'
  return message
}

export function MicroLabel({ children }) {
  return <p className="micro-label">{children}</p>
}

// Gradient page banner shared by every page except System, which has its own live hero.
export function PageHeader({ id, eyebrow, title, description, className, children }) {
  return (
    <header className={cn('page-header', className)}>
      <span className="page-header-orb" aria-hidden="true" />
      <span className="page-header-orb page-header-orb-2" aria-hidden="true" />
      {eyebrow ? <p className="page-header-eyebrow">{eyebrow}</p> : null}
      <h1 id={id}>{title}</h1>
      {description ? <p>{description}</p> : null}
      {children}
    </header>
  )
}

const STATUS_BADGE_TONES = { COMPLETED: 'success', FAILED: 'danger', RUNNING: 'info', QUEUED: 'warning' }

// Status is always conveyed as text; the colour only reinforces it.
export function StatusBadge({ status, fallback = 'Unknown' }) {
  const normalized = (status || '').toUpperCase()
  const tone = STATUS_BADGE_TONES[normalized] || 'neutral'
  return <span className={cn('badge', `badge-${tone}`)}>{normalized || fallback}</span>
}

export function SectionHeading({ eyebrow, title, description, anchor }) {
  return (
    <div className="section-heading">
      {eyebrow ? <MicroLabel>{eyebrow}</MicroLabel> : null}
      <div>
        {anchor ? <h2 id={anchor}>{title}</h2> : <h2>{title}</h2>}
        {description ? <p>{description}</p> : null}
      </div>
    </div>
  )
}

export function JobMetaCard({ title, value }) {
  return <div className="meta-card"><MicroLabel>{title}</MicroLabel><div className="meta-value">{value}</div></div>
}

export function ErrorPanel({ message }) {
  if (!message) return null
  return (
    <section className="feedback-panel error-panel" aria-live="polite">
      <h2 className="feedback-title">Analysis did not complete</h2>
      <p>{getErrorMessage(message)}</p>
    </section>
  )
}

export function LoadingSkeleton({ phase }) {
  return (
    <section className="feedback-panel loading-panel" aria-live="polite">
      <h2 className="feedback-title">{phase === 'submitting' ? 'Creating job and queueing work' : 'Worker pipeline is processing the repository'}</h2>
      <div className="skeleton-grid" aria-hidden="true"><div className="skeleton-card" /><div className="skeleton-card" /><div className="skeleton-card" /><div className="skeleton-card" /></div>
    </section>
  )
}
