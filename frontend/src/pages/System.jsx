import { useEffect, useRef, useState } from 'react'
import { durationBetween, formatDuration, formatNumber, parseRepoFromUrl } from '../lib/format.js'
import { useSystemMetrics } from '../hooks/useSystemMetrics.js'
import { StatusBadge } from '../components/ui.jsx'
import {
  AnimatedNumber,
  AttemptDots,
  ColumnChart,
  Donut,
  JobTrack,
  Legend,
  LivePill,
  Panel,
  QueueTube,
  RingGauge,
  RouteBars,
  StatTile,
  SystemMap,
  WorkerPool,
} from '../components/SystemVisuals.jsx'

const LIVE_PIPELINE_STAGES = [
  ['Submitted', 'browser'],
  ['Job created', 'box'],
  ['Queued', 'queue'],
  ['Worker', 'workers'],
  ['Analysis', 'analysis'],
  ['Persisted', 'database'],
  ['Complete', 'layers'],
]

const COLORS = {
  completed: '#16a34a',
  failed: '#dc2626',
  running: '#0d9488',
}

const STATUS_COPY = {
  idle: 'Submit a repository to run the production analysis pipeline.',
  submitting: 'Submitting repository for analysis...',
  polling: 'Polling the live job status from the backend worker system...',
  done: 'Analysis complete.',
  error: 'The latest request did not complete successfully.',
}

const isNumber = (value) => typeof value === 'number' && Number.isFinite(value)

function formatTime(value) {
  if (!value) return null
  const date = new Date(value)
  return Number.isNaN(date.getTime()) ? null : date.toLocaleTimeString()
}

function resolveLiveStageState(index, phase, job, analysis) {
  const status = (job?.status || '').toUpperCase()
  const hasJob = Boolean(job?.jobId)
  const hasAnalysis = Boolean(analysis)
  if (index === 0) return phase === 'idle' ? 'idle' : 'done'
  if (index === 1) return hasJob ? 'done' : phase === 'submitting' ? 'active' : 'idle'
  if (index === 2) {
    if (status === 'QUEUED') return 'active'
    if (status === 'RUNNING' || status === 'COMPLETED' || status === 'FAILED') return 'done'
    return hasJob ? 'done' : 'idle'
  }
  if (index === 3) {
    if (status === 'RUNNING') return 'active'
    if (status === 'COMPLETED' || status === 'FAILED') return 'done'
    return 'idle'
  }
  if (index === 4) {
    if (status === 'RUNNING') return 'active'
    if (status === 'COMPLETED' || status === 'FAILED') return 'done'
    return 'idle'
  }
  if (index === 5) {
    if (hasAnalysis) return 'done'
    if (status === 'COMPLETED') return 'active'
    return 'idle'
  }
  if (index === 6) {
    if (phase === 'done') return 'done'
    if (phase === 'error' && status === 'FAILED') return 'error'
    return 'idle'
  }
  return 'idle'
}

function deriveRetryState(jobDetails) {
  const attempt = jobDetails?.reliability?.attempt
  const maxRetries = jobDetails?.reliability?.maxRetries
  const deadLettered = jobDetails?.reliability?.deadLettered
  const status = jobDetails?.job?.status

  if (deadLettered) return 'Dead-lettered'
  if (typeof attempt !== 'number') return 'Unavailable'
  if (status === 'FAILED') return 'Failed'
  if (status === 'RUNNING' && attempt > 1) return `Retrying (${attempt - 1}/${maxRetries ?? '?'})`
  if (status === 'COMPLETED' && attempt > 1) return `Completed after ${attempt - 1} retries`
  return 'No retry observed'
}

function buildTimeline(jobDetails) {
  if (!jobDetails?.job) return []

  const timeline = [
    { label: 'Submitted', timestamp: jobDetails.job.createdAt, note: 'Recorded', tone: 'info' },
  ]

  if (jobDetails.queue?.streamId) {
    timeline.push({ label: 'Queued', timestamp: null, note: 'In stream', tone: 'queue' })
  }

  if (jobDetails.job.startedAt) {
    timeline.push({ label: 'Processing', timestamp: jobDetails.job.startedAt, note: 'Started', tone: 'analysis' })
  }

  if (jobDetails.job.status === 'COMPLETED') {
    timeline.push({ label: 'Complete', timestamp: jobDetails.job.completedAt, note: 'Stored', tone: 'done' })
  }

  if (jobDetails.job.status === 'FAILED') {
    timeline.push({ label: 'Failed', timestamp: jobDetails.job.completedAt, note: 'Failed', tone: 'error' })
  }

  if (jobDetails.reliability?.deadLettered) {
    timeline.push({ label: 'Dead-lettered', timestamp: null, note: 'In dead-letter stream', tone: 'error' })
  }

  return timeline
}

function CurrentJob({ phase, job, analysis, statusMessage, jobDetails, expanded, onToggle }) {
  const stages = LIVE_PIPELINE_STAGES.map(([label, icon], index) => ({
    label,
    icon,
    state: resolveLiveStageState(index, phase, job, analysis),
  }))
  const durationMs = durationBetween(job?.createdAt, job?.completedAt) || durationBetween(job?.createdAt, job?.startedAt)
  return (
    <Panel layer="frontend" icon="route" title="Current job" aside={<StatusBadge status={job?.status} fallback="Idle" />}>
      <JobTrack stages={stages} />
      <div className="sys-job-bar">
        <p className="sys-job-status" aria-live="polite">{statusMessage || STATUS_COPY[phase]}</p>
        {job?.jobId ? (
          <>
            <span className="sys-chip mono" title={job.jobId}>{job.jobId.slice(0, 8)}</span>
            <span className="sys-chip">{durationMs ? formatDuration(durationMs) : 'In progress'}</span>
            <AttemptDots attempt={jobDetails?.reliability?.attempt} maxRetries={jobDetails?.reliability?.maxRetries} />
            <button
              type="button"
              className="button-secondary button-small"
              onClick={onToggle}
              aria-expanded={expanded}
              aria-controls={expanded ? 'job-detail' : undefined}
            >
              {expanded ? 'Hide detail' : 'Show detail'}
            </button>
          </>
        ) : (
          <a className="button-secondary button-small" href="#/">Analyse a repository</a>
        )}
      </div>
    </Panel>
  )
}

function JobDetail({ job, jobDetails, submittedUrl, expanded }) {
  if (!job?.jobId || !expanded) return null
  const detailJob = jobDetails?.job || {
    id: job.jobId,
    repositoryUrl: job.repositoryUrl,
    status: job.status,
    createdAt: job.createdAt,
    startedAt: job.startedAt,
    completedAt: job.completedAt,
    durationMs: durationBetween(job.createdAt, job.completedAt),
  }
  const timeline = buildTimeline(jobDetails)
  const duration = typeof detailJob.durationMs === 'number' ? detailJob.durationMs : durationBetween(detailJob.createdAt, detailJob.completedAt)
  const repo = parseRepoFromUrl(detailJob.repositoryUrl || submittedUrl)
  const traceId = jobDetails?.observability?.traceId
  const traceAvailable = jobDetails?.observability?.traceAvailable === true
  const failureReason = jobDetails?.reliability?.failureReason
  const attempt = jobDetails?.reliability?.attempt
  const deadLettered = jobDetails?.reliability?.deadLettered
  const retryable = jobDetails?.reliability?.retryable
  const retryState = deriveRetryState(jobDetails)
  const facts = [
    ['Repository', repo?.fullName || detailJob.repositoryUrl || 'Unavailable'],
    ['Worker', jobDetails?.queue?.consumer || 'Unavailable'],
    ['Duration', duration ? formatDuration(duration) : 'In progress'],
    ['Attempts', <AttemptDots key="attempts" attempt={attempt} maxRetries={jobDetails?.reliability?.maxRetries} />],
    ['Retry state', retryState],
    ['Dead-lettered', typeof deadLettered === 'boolean' ? (deadLettered ? 'Yes' : 'No') : 'Unavailable'],
    ['Retryable', typeof retryable === 'boolean' ? (retryable ? 'Yes' : 'No') : 'Unavailable'],
    ['Trace ID', traceAvailable ? <span key="trace" className="mono">{traceId}</span> : 'Unavailable'],
  ]
  if (detailJob.status === 'FAILED') facts.push(['Failure reason', failureReason || 'Unavailable'])

  return (
    <Panel id="job-detail" layer="api" icon="trace" title="Job record" aside={<span className="sys-chip mono">{detailJob.id || job.jobId}</span>}>
      <ol className="sys-timeline">
        {timeline.map((item) => (
          <li key={`${item.label}-${item.timestamp || item.note}`} className={`sys-timeline-item is-${item.tone}`}>
            <span className="sys-timeline-dot" aria-hidden="true" />
            <span className="sys-timeline-label">{item.label}</span>
            <span className="sys-timeline-time">{formatTime(item.timestamp) || item.note}</span>
          </li>
        ))}
      </ol>
      <dl className="sys-facts">
        {facts.map(([label, value]) => (
          <div key={label} className="sys-fact">
            <dt>{label}</dt>
            <dd>{value}</dd>
          </div>
        ))}
      </dl>
    </Panel>
  )
}

// Prefer the system summary; fall back to the Prometheus counters.
function collectStats(summary, metrics) {
  const overview = metrics?.overview
  const jobs = summary?.jobs
  const pick = (key, fallbackKey) => (isNumber(jobs?.[key]) ? jobs[key] : overview?.[fallbackKey])
  const pending = summary?.queue?.pending
  const lag = summary?.queue?.lag
  const routes = metrics?.routeCounts || []
  return {
    submitted: pick('submitted', 'jobsSubmitted'),
    completed: pick('completed', 'jobsCompleted'),
    failed: pick('failed', 'jobsFailed'),
    retried: pick('retried', 'jobsRetried'),
    deadLettered: pick('deadLettered', 'jobsDeadLettered'),
    active: pick('active', 'jobsActive'),
    pending,
    lag,
    queued: isNumber(pending) && isNumber(lag) ? pending + lag : overview?.queueDepth,
    workersActive: summary?.workers?.active,
    workersConfigured: summary?.workers?.configured,
    consumers: summary?.queue?.consumers,
    stream: summary?.queue?.stream,
    successRate: summary?.reliability?.successRate ?? overview?.successRate,
    failureRate: summary?.reliability?.failureRate ?? overview?.failureRate,
    averageSeconds: overview?.averageProcessingSeconds,
    buckets: metrics?.durationBuckets || [],
    routes: routes.slice(0, 6),
    httpTotal: metrics ? routes.reduce((sum, route) => sum + route.count, 0) : undefined,
    observability: summary?.observability,
  }
}

function LiveMap({ stats, flowing }) {
  const nodes = [
    { id: 'api', layer: 'api', icon: 'server', name: 'API', value: <AnimatedNumber value={stats.httpTotal} />, caption: 'HTTP requests' },
    { id: 'stream', layer: 'queue', icon: 'queue', name: 'Redis stream', value: <AnimatedNumber value={stats.queued} />, caption: 'waiting', busy: stats.queued > 0 },
    {
      id: 'workers',
      layer: 'queue',
      icon: 'workers',
      name: 'Workers',
      value: isNumber(stats.workersActive) ? <><AnimatedNumber value={stats.workersActive} />/{stats.workersConfigured ?? '?'}</> : '—',
      caption: 'online',
    },
    { id: 'analysis', layer: 'analysis', icon: 'analysis', name: 'Analysis', value: <AnimatedNumber value={stats.active} />, caption: 'running', busy: stats.active > 0 },
    { id: 'results', layer: 'data', icon: 'database', name: 'Results', value: <AnimatedNumber value={stats.completed} />, caption: 'completed' },
  ]
  return <SystemMap nodes={nodes} flowing={flowing} />
}

function OutcomesPanel({ stats }) {
  const segments = [
    { label: 'Completed', value: stats.completed || 0, color: COLORS.completed },
    { label: 'Failed', value: stats.failed || 0, color: COLORS.failed },
    { label: 'Running', value: stats.active || 0, color: COLORS.running },
  ]
  const summary = segments.map((segment) => `${segment.value} ${segment.label.toLowerCase()}`).join(', ')
  return (
    <Panel layer="data" icon="check" title="Job outcomes" aside="since restart">
      <div className="sys-outcomes">
        <Donut segments={segments} total={stats.submitted} label={`Job outcomes: ${summary}, of ${stats.submitted ?? 0} submitted`}>
          <strong><AnimatedNumber value={stats.submitted} /></strong>
          <span>submitted</span>
        </Donut>
        <Legend items={segments} />
      </div>
    </Panel>
  )
}

function ReliabilityPanel({ stats }) {
  return (
    <Panel layer="analysis" icon="shield" title="Reliability">
      <div className="sys-gauges">
        <RingGauge value={stats.successRate} label="Success rate" layer="data" />
        <RingGauge value={stats.failureRate} label="Failure rate" layer="danger" />
      </div>
      <div className="sys-stats">
        <StatTile layer="warning" icon="retry" label="Retried" value={stats.retried} />
        <StatTile layer="danger" icon="inbox" label="Dead-lettered" value={stats.deadLettered} />
        <StatTile layer="analysis" icon="gauge" label="Avg time" value={stats.averageSeconds} format={(n) => `${n.toFixed(2)} s`} />
      </div>
    </Panel>
  )
}

function WorkersPanel({ stats }) {
  return (
    <Panel layer="queue" icon="workers" title="Workers & queue" aside={stats.stream ? <span className="mono">{stats.stream}</span> : null}>
      <WorkerPool configured={stats.workersConfigured} active={stats.workersActive} busy={stats.active} />
      <QueueTube pending={stats.pending} lag={stats.lag} />
      <div className="sys-stats">
        <StatTile layer="queue" icon="inbox" label="Pending" value={stats.pending} />
        <StatTile layer="queue" icon="queue" label="Lag" value={stats.lag} />
        <StatTile layer="queue" icon="workers" label="Consumers" value={stats.consumers} />
      </div>
    </Panel>
  )
}

function ThroughputPanel({ stats }) {
  const items = [
    { label: 'Submitted', value: stats.submitted, layer: 'api' },
    { label: 'Completed', value: stats.completed, layer: 'data' },
    { label: 'Failed', value: stats.failed, layer: 'danger' },
    { label: 'Retried', value: stats.retried, layer: 'warning' },
    { label: 'Dead-lettered', value: stats.deadLettered, layer: 'observability' },
  ]
  const label = `Job throughput: ${items.map((item) => `${item.value ?? 0} ${item.label.toLowerCase()}`).join(', ')}`
  return (
    <Panel layer="api" icon="chart" title="Job throughput" aside="since restart">
      <ColumnChart items={items} label={label} />
    </Panel>
  )
}

function LatencyPanel({ buckets }) {
  const measured = buckets.reduce((sum, bucket) => sum + bucket.count, 0)
  const items = buckets.map((bucket) => ({ label: `≤${bucket.le}s`, value: bucket.count, layer: 'analysis' }))
  return (
    <Panel layer="analysis" icon="gauge" title="Processing time" aside={`${formatNumber(measured)} measured`}>
      <ColumnChart compact items={items} label={`Processing time histogram: ${measured} jobs measured`} />
    </Panel>
  )
}

function TrafficPanel({ routes, flags }) {
  const aside = flags ? (
    <span className="sys-flags">
      {[['Metrics', flags.metrics], ['Tracing', flags.tracing]].map(([label, on]) => (
        <span key={label} className={`sys-flag ${on ? 'is-on' : 'is-off'}`}>{label} {on ? 'on' : 'off'}</span>
      ))}
    </span>
  ) : null
  return (
    <Panel layer="observability" icon="globe" title="HTTP traffic" aside={aside}>
      <RouteBars routes={routes} />
    </Panel>
  )
}

export function SystemPage({ analysisJob }) {
  const { phase, job, analysis, statusMessage, jobDetails, submittedUrl } = analysisJob
  const [jobDetailOpen, setJobDetailOpen] = useState(true)
  // Metrics are polled only while this page is mounted (see useSystemMetrics).
  const { metricsState, systemState, refreshMetrics, refreshSystemSummary } = useSystemMetrics()

  // Refresh immediately when the current job changes state, as the single-page version did,
  // instead of waiting for the next 15 s interval.
  const activityKey = `${job?.jobId || ''}:${job?.status || ''}:${phase}`
  const lastActivityKey = useRef(activityKey)
  useEffect(() => {
    if (lastActivityKey.current === activityKey) return
    lastActivityKey.current = activityKey
    refreshMetrics()
    refreshSystemSummary()
  }, [activityKey, refreshMetrics, refreshSystemSummary])

  const stats = collectStats(systemState.data, metricsState.data)
  const online = Boolean(systemState.data) && !systemState.error
  const jobStatus = (job?.status || '').toUpperCase()
  const flowing = stats.active > 0 || stats.queued > 0 || phase === 'submitting' || jobStatus === 'QUEUED' || jobStatus === 'RUNNING'

  return (
    <div className="sys">
      <header className="sys-hero">
        <div className="sys-hero-head">
          <div>
            <h1>System</h1>
            <p>Live backend telemetry, refreshed every 15 seconds.</p>
          </div>
          <LivePill online={online} updatedAt={systemState.lastUpdated} />
        </div>
        <LiveMap stats={stats} flowing={flowing} />
        {metricsState.error ? <p className="panel-note error">{metricsState.error}</p> : null}
        {systemState.error ? <p className="panel-note error">{systemState.error}</p> : null}
      </header>
      <CurrentJob phase={phase} job={job} analysis={analysis} statusMessage={statusMessage} jobDetails={jobDetails} expanded={jobDetailOpen} onToggle={() => setJobDetailOpen((current) => !current)} />
      <JobDetail job={job} jobDetails={jobDetails} submittedUrl={submittedUrl} expanded={jobDetailOpen} />
      <div className="sys-grid sys-grid-3">
        <OutcomesPanel stats={stats} />
        <ReliabilityPanel stats={stats} />
        <WorkersPanel stats={stats} />
      </div>
      <div className="sys-grid sys-grid-2">
        <ThroughputPanel stats={stats} />
        <LatencyPanel buckets={stats.buckets} />
      </div>
      <TrafficPanel routes={stats.routes} flags={stats.observability} />
    </div>
  )
}
