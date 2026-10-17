import { useEffect, useRef, useState } from 'react'
import { cn } from './ui.jsx'
import { LayerIcon } from './ArchitectureVisuals.jsx'
import { formatNumber } from '../lib/format.js'

// Animated, chart-first building blocks for the System page. Every chart has a
// visible number or an aria-label, so colour and motion never carry meaning alone.

function prefersReducedMotion() {
  return typeof window !== 'undefined' && window.matchMedia?.('(prefers-reduced-motion: reduce)').matches
}

const isNumber = (value) => typeof value === 'number' && Number.isFinite(value)

// Counts from the previous value to the new one whenever live data refreshes.
export function AnimatedNumber({ value, format = formatNumber, fallback = '—' }) {
  const [shown, setShown] = useState(0)
  const current = useRef(0)

  useEffect(() => {
    if (!isNumber(value)) return undefined
    const from = current.current
    if (prefersReducedMotion() || from === value) {
      current.current = value
      setShown(value)
      return undefined
    }
    let frame
    const start = performance.now()
    const tick = (now) => {
      const t = Math.min(1, (now - start) / 700)
      const next = from + (value - from) * (1 - (1 - t) ** 3)
      current.current = next
      setShown(next)
      if (t < 1) frame = requestAnimationFrame(tick)
    }
    frame = requestAnimationFrame(tick)
    return () => cancelAnimationFrame(frame)
  }, [value])

  if (!isNumber(value)) return fallback
  return format(Number.isInteger(value) ? Math.round(shown) : shown)
}

export function LivePill({ online, updatedAt }) {
  const time = updatedAt ? new Date(updatedAt).toLocaleTimeString() : null
  return (
    <span className={cn('sys-live', online ? 'is-online' : 'is-offline')}>
      <span className="sys-live-dot" aria-hidden="true" />
      {online ? 'Live' : 'Offline'}
      {time ? <span className="sys-live-time">· updated {time}</span> : null}
    </span>
  )
}

export function Panel({ id, layer, icon, title, aside, className, children }) {
  return (
    <section id={id} className={cn('sys-panel', `layer-${layer}`, className)}>
      <header className="sys-panel-head">
        <span className="sys-panel-icon" aria-hidden="true"><LayerIcon name={icon} size={18} /></span>
        <h2>{title}</h2>
        {aside ? <span className="sys-panel-aside">{aside}</span> : null}
      </header>
      {children}
    </section>
  )
}

// Live system map: API -> stream -> workers -> analysis -> results.
// Packets only travel along the links while work is actually in flight.
export function SystemMap({ nodes, flowing }) {
  return (
    <ol className={cn('sys-map', flowing && 'is-flowing')} style={{ '--n': nodes.length }} aria-label="Live system map">
      {nodes.map((node, index) => (
        <li key={node.id} className={cn('sys-map-item', `layer-${node.layer}`, node.busy && 'is-busy')}>
          {index > 0 ? (
            <span className="sys-link" aria-hidden="true">
              <span className="sys-packet" />
              <span className="sys-packet" />
              <span className="sys-packet" />
            </span>
          ) : null}
          <span className="sys-map-orb" aria-hidden="true"><LayerIcon name={node.icon} size={24} /></span>
          <span className="sys-map-name">{node.name}</span>
          <strong className="sys-map-value">{node.value}</strong>
          <span className="sys-map-caption">{node.caption}</span>
        </li>
      ))}
    </ol>
  )
}

const TRACK_STATE_TEXT = { done: 'complete', active: 'in progress', error: 'failed', idle: 'pending' }

export function JobTrack({ stages }) {
  const reached = stages.reduce((last, stage, index) => (stage.state === 'idle' ? last : index), 0)
  const progress = stages.length > 1 ? reached / (stages.length - 1) : 0
  return (
    <div className="sys-track" style={{ '--n': stages.length, '--progress': progress }}>
      <span className="sys-track-rail" aria-hidden="true"><span className="sys-track-fill" /></span>
      <ol className="sys-track-stages" aria-label="Current job progress">
        {stages.map((stage) => {
          const icon = stage.state === 'done' ? 'check' : stage.state === 'error' ? 'alert' : stage.icon
          return (
            <li key={stage.label} className={cn('sys-track-stage', `is-${stage.state}`)}>
              <span className="sys-track-node" aria-hidden="true"><LayerIcon name={icon} size={18} /></span>
              <span className="sys-track-label">
                {stage.label}
                <span className="sr-only">: {TRACK_STATE_TEXT[stage.state]}</span>
              </span>
            </li>
          )
        })}
      </ol>
    </div>
  )
}

export function Donut({ segments, total, label, children, size = 168, stroke = 18 }) {
  const radius = (size - stroke) / 2
  const circumference = 2 * Math.PI * radius
  const sum = segments.reduce((acc, segment) => acc + segment.value, 0)
  const denominator = Math.max(total || 0, sum)
  let offset = 0
  const arcs = denominator > 0
    ? segments.filter((segment) => segment.value > 0).map((segment) => {
      const length = (segment.value / denominator) * circumference
      const arc = { ...segment, length, offset }
      offset += length
      return arc
    })
    : []

  return (
    <div className="sys-donut" style={{ width: size, height: size }}>
      <svg viewBox={`0 0 ${size} ${size}`} role="img" aria-label={label}>
        <circle className="sys-donut-track" cx={size / 2} cy={size / 2} r={radius} strokeWidth={stroke} />
        {arcs.map((arc) => (
          <circle
            key={arc.label}
            className="sys-donut-arc"
            cx={size / 2}
            cy={size / 2}
            r={radius}
            strokeWidth={stroke}
            style={{ stroke: arc.color, strokeDasharray: `${arc.length} ${circumference - arc.length}`, strokeDashoffset: -arc.offset }}
          />
        ))}
      </svg>
      <div className="sys-donut-center">{children}</div>
    </div>
  )
}

export function Legend({ items }) {
  return (
    <ul className="sys-legend">
      {items.map((item) => (
        <li key={item.label}>
          <span className="sys-legend-swatch" style={{ background: item.color }} aria-hidden="true" />
          <span className="sys-legend-label">{item.label}</span>
          <strong><AnimatedNumber value={item.value} /></strong>
        </li>
      ))}
    </ul>
  )
}

export function RingGauge({ value, label, layer, size = 112, stroke = 10 }) {
  const radius = (size - stroke) / 2
  const circumference = 2 * Math.PI * radius
  const known = isNumber(value)
  const fill = known ? (Math.min(100, Math.max(0, value)) / 100) * circumference : 0
  return (
    <figure className={cn('sys-gauge', `layer-${layer}`)}>
      <div className="sys-gauge-ring" style={{ width: size, height: size }}>
        <svg viewBox={`0 0 ${size} ${size}`} aria-hidden="true" focusable="false">
          <circle className="sys-gauge-track" cx={size / 2} cy={size / 2} r={radius} strokeWidth={stroke} />
          {fill > 0 ? (
            <circle
              className="sys-gauge-arc"
              cx={size / 2}
              cy={size / 2}
              r={radius}
              strokeWidth={stroke}
              style={{ strokeDasharray: `${fill} ${circumference - fill}` }}
            />
          ) : null}
        </svg>
        <span className="sys-gauge-value">
          {known ? <AnimatedNumber value={value} format={(n) => `${n.toFixed(0)}%`} /> : <>—<span className="sr-only"> no data yet</span></>}
        </span>
      </div>
      <figcaption>{label}</figcaption>
    </figure>
  )
}

export function StatTile({ layer, icon, label, value, format }) {
  return (
    <div className={cn('sys-stat', `layer-${layer}`)}>
      <span className="sys-stat-icon" aria-hidden="true"><LayerIcon name={icon} size={16} /></span>
      <strong className="sys-stat-value"><AnimatedNumber value={value} format={format} /></strong>
      <span className="sys-stat-label">{label}</span>
    </div>
  )
}

// Vertical columns, used for both job throughput and the latency histogram.
export function ColumnChart({ items, label, compact = false }) {
  const max = Math.max(1, ...items.map((item) => item.value || 0))
  return (
    <div className={cn('sys-columns', compact && 'is-compact')} role="img" aria-label={label}>
      {items.map((item, index) => (
        <div key={item.label} className={cn('sys-column', `layer-${item.layer}`)} style={{ '--i': index }}>
          <span className="sys-column-value"><AnimatedNumber value={item.value} /></span>
          <span className="sys-column-plot">
            <span className="sys-column-bar" style={{ height: `${((item.value || 0) / max) * 100}%` }} />
          </span>
          <span className="sys-column-label">{item.label}</span>
        </div>
      ))}
    </div>
  )
}

export function WorkerPool({ configured, active, busy }) {
  const total = Math.min(16, Math.max(configured || 0, active || 0))
  if (!total) return <p className="sys-empty">Worker data unavailable</p>
  return (
    <ul className="sys-workers" aria-label={`${active ?? 0} of ${configured ?? '?'} workers online, ${busy ?? 0} jobs in progress`}>
      {Array.from({ length: total }, (_, index) => {
        const state = index < (busy || 0) ? 'busy' : index < (active || 0) ? 'online' : 'offline'
        return (
          <li key={index} className={cn('sys-worker', `is-${state}`)} style={{ '--i': index }}>
            <span className="sys-worker-chip" aria-hidden="true"><LayerIcon name="workers" size={20} /></span>
            <span className="sys-worker-name">W{index + 1}</span>
          </li>
        )
      })}
    </ul>
  )
}

const MAX_VISIBLE_MESSAGES = 12

export function QueueTube({ pending, lag }) {
  const known = isNumber(pending) && isNumber(lag)
  const total = known ? pending + lag : null
  const visible = Math.min(total || 0, MAX_VISIBLE_MESSAGES)
  return (
    <div className={cn('sys-tube', total > 0 && 'is-moving')} role="img" aria-label={known ? `${total} messages waiting in the stream` : 'Queue depth unavailable'}>
      <span className="sys-tube-cap" aria-hidden="true" />
      <span className="sys-tube-pipe">
        {Array.from({ length: visible }, (_, index) => <span key={index} className="sys-msg" style={{ '--i': index }} />)}
        {total > MAX_VISIBLE_MESSAGES ? <span className="sys-tube-more">+{total - MAX_VISIBLE_MESSAGES}</span> : null}
        {total === 0 ? <span className="sys-tube-empty">empty</span> : null}
        {!known ? <span className="sys-tube-empty">unavailable</span> : null}
      </span>
      <span className="sys-tube-cap" aria-hidden="true" />
    </div>
  )
}

function statusTone(status) {
  const code = Number.parseInt(status, 10)
  if (code >= 500) return 'danger'
  if (code >= 400) return 'warning'
  return 'data'
}

export function RouteBars({ routes }) {
  if (!routes.length) return <p className="sys-empty">No HTTP traffic recorded yet</p>
  const max = Math.max(1, ...routes.map((route) => route.count))
  return (
    <ul className="sys-routes">
      {routes.map((route, index) => (
        <li key={`${route.method}-${route.route}-${route.status}`} className={cn('sys-route', `layer-${statusTone(route.status)}`)} style={{ '--i': index }}>
          <span className="sys-route-method">{route.method}</span>
          <span className="sys-route-path mono">{route.route}</span>
          <span className="sys-route-status">{route.status}</span>
          <span className="sys-route-track" aria-hidden="true">
            <span className="sys-route-fill" style={{ width: `${(route.count / max) * 100}%` }} />
          </span>
          <strong className="sys-route-count"><AnimatedNumber value={route.count} /></strong>
        </li>
      ))}
    </ul>
  )
}

export function AttemptDots({ attempt, maxRetries }) {
  if (!isNumber(attempt)) return <span className="sys-muted">Unavailable</span>
  const slots = Math.max(attempt, isNumber(maxRetries) ? maxRetries + 1 : attempt)
  return (
    <span className="sys-attempts" role="img" aria-label={`Attempt ${attempt} of ${slots}`}>
      {Array.from({ length: slots }, (_, index) => (
        <span key={index} className={cn('sys-attempt', index < attempt && 'is-used')} />
      ))}
      <span className="sys-attempts-text" aria-hidden="true">{attempt}/{slots}</span>
    </span>
  )
}
