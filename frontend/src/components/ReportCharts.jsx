import { formatNumber, formatPercent } from '../lib/format.js'
import { cn } from './ui.jsx'

// Small CSS/SVG chart primitives for the analysis report. Every chart is paired with
// visible text values, so colour and shape never carry information on their own.

const GRADE_TONES = { A: 'success', B: 'success', C: 'warning', D: 'danger', F: 'danger' }

export function gradeTone(grade) {
  return GRADE_TONES[String(grade || '').trim().charAt(0).toUpperCase()] || 'neutral'
}

export function GradeBadge({ grade, compact = false }) {
  if (!grade) return null
  return (
    <span className={cn('badge', 'badge-plain', `badge-${gradeTone(grade)}`)}>
      {compact ? <><span className="sr-only">Grade </span>{grade}</> : `Grade ${grade}`}
    </span>
  )
}

function clampRatio(value, max) {
  if (typeof value !== 'number' || typeof max !== 'number' || max <= 0) return 0
  return Math.min(Math.max(value / max, 0), 1)
}

const GAUGE_RADIUS = 52
const GAUGE_CIRCUMFERENCE = 2 * Math.PI * GAUGE_RADIUS

export function ScoreGauge({ score, max = 100, grade, size = 'md' }) {
  if (typeof score !== 'number') return null
  const filled = clampRatio(score, max) * GAUGE_CIRCUMFERENCE
  const label = `Health score ${score} out of ${max}${grade ? `, grade ${grade}` : ''}`
  return (
    <svg className={cn('score-gauge', `score-gauge-${size}`)} viewBox="0 0 120 120" role="img" aria-label={label}>
      <circle className="gauge-track" cx="60" cy="60" r={GAUGE_RADIUS} />
      <circle
        className="gauge-fill"
        cx="60"
        cy="60"
        r={GAUGE_RADIUS}
        strokeDasharray={`${filled} ${GAUGE_CIRCUMFERENCE}`}
        transform="rotate(-90 60 60)"
      />
      <text className="gauge-value" x="60" y="58" textAnchor="middle" dominantBaseline="middle">{score}</text>
      <text className="gauge-max" x="60" y="82" textAnchor="middle" dominantBaseline="middle">of {max}</text>
    </svg>
  )
}

// Decorative bar; callers always render the numeric value next to it.
export function Meter({ value, max = 100, size = 'md' }) {
  return (
    <span className={cn('bar-track', 'meter', `meter-${size}`)} aria-hidden="true">
      <span className="bar-fill" style={{ width: `${clampRatio(value, max) * 100}%` }} />
    </span>
  )
}

export function extensionLabel(ext) {
  return ext === '(none)' ? 'No extension' : ext
}

// Rows come from the `languages` map (extension -> file count), sorted by count.
export function buildExtensionRows(languages) {
  const entries = Object.entries(languages || {})
    .filter(([, count]) => typeof count === 'number' && count > 0)
    .sort((a, b) => b[1] - a[1])
  const total = entries.reduce((sum, [, count]) => sum + count, 0)
  return {
    total,
    rows: entries.map(([ext, count]) => ({ key: ext, label: extensionLabel(ext), count, percent: (count / total) * 100 })),
  }
}

// Keeps the largest `limit` rows and folds the remainder into a single "Other" segment.
export function topSegments(rows, total, limit) {
  if (rows.length <= limit + 1) return rows
  const head = rows.slice(0, limit)
  const rest = rows.slice(limit)
  const count = rest.reduce((sum, row) => sum + row.count, 0)
  return [...head, { key: '__other', label: `Other (${rest.length} types)`, count, percent: (count / total) * 100, isOther: true }]
}

export function fileLabel(count) {
  return `${formatNumber(count)} ${count === 1 ? 'file' : 'files'}`
}

// The legend carries the data in text; pass showValues={false} when a full table follows.
export function DistributionBar({ segments, caption, showValues = true }) {
  if (!segments.length) return null
  let toneIndex = 0
  const toned = segments.map((segment) => ({ ...segment, tone: segment.isOther ? 'other' : String(toneIndex++) }))
  return (
    <figure className="distribution">
      <div className="distribution-bar" aria-hidden="true">
        {toned.map((segment) => (
          <span key={segment.key} className={`distribution-segment tone-${segment.tone}`} style={{ width: `${segment.percent}%` }} />
        ))}
      </div>
      <ul className="distribution-legend" aria-label={caption}>
        {toned.map((segment, index) => (
          <li key={segment.key} style={{ '--i': index }}>
            <span className={`legend-swatch tone-${segment.tone}`} aria-hidden="true" />
            <span className={segment.isOther || segment.key === '(none)' ? '' : 'mono'}>{segment.label}</span>
            {showValues ? <span className="legend-value">{fileLabel(segment.count)} · {formatPercent(segment.percent, 0)}</span> : null}
          </li>
        ))}
      </ul>
    </figure>
  )
}
