import { useCallback, useEffect, useMemo, useRef, useState } from 'react'

const getApiUrl = () => {
  const envUrl = import.meta.env.VITE_API_URL
  if (envUrl) return envUrl
  if (typeof window !== 'undefined' && window.location.hostname.includes('vercel.app')) {
    return 'https://cortex-code-intelligence-platform.onrender.com'
  }
  return 'http://localhost:8080'
}

const API = getApiUrl()
const EXAMPLE_REPOSITORY = 'https://github.com/octocat/Hello-World'

const PROFILE_LINKS = {
  github: 'https://github.com/gavinspet',
  linkedin: 'https://www.linkedin.com/in/kartick-kumar-ghosh-779679190/',
}

const PROJECT_LINKS = {
  repo: 'https://github.com/gavinspet/Cortex-Code-Intelligence-Platform',
  health: `${API}/health`,
  metrics: `${API}/metrics`,
}

const SUPPORTED_REPO_HOST_PATTERNS = ['github.com', 'gitlab.com', 'bitbucket.org', 'dev.azure.com']

const TECH_GROUPS = [
  ['frameworks', 'Detected technologies'],
  ['frontendFrameworks', 'Frontend frameworks'],
  ['backendFrameworks', 'Backend frameworks'],
  ['buildSystems', 'Build tooling'],
  ['packageManagers', 'Package managers'],
  ['testingFrameworks', 'Testing'],
  ['ciSystems', 'CI / CD'],
  ['containers', 'Containers'],
  ['cloudProviders', 'Cloud'],
  ['databases', 'Databases'],
]

const CAPABILITY_CARDS = [
  ['Repository Intelligence', 'Turn a public repository into a structured engineering brief with file metrics, health signals, and repository context.', 'cyan'],
  ['Architecture Analysis', 'Surface repository shape, project maturity, complexity, and design signals from static code inspection.', 'teal'],
  ['Language & Technology Detection', 'Identify frameworks, build systems, package managers, testing stacks, containers, and cloud deployment hints.', 'amber'],
  ['Engineering Health', 'Score documentation, testing, CI/CD, maintainability, configuration, security, and project structure.', 'green'],
  ['Engineering Insights', 'Generate AI-style repository briefings from deterministic static analysis and GitHub metadata.', 'violet'],
  ['GitHub Intelligence', 'Promote stars, forks, watchers, issues, ownership, branches, license, and topics to first-class product signals.', 'slate'],
  ['Distributed Job Processing', 'Submission and analysis execute asynchronously through the production worker pipeline instead of blocking the browser.', 'cyan'],
  ['Reliability Controls', 'Showcase retry boundaries, dead-letter handling, persistent job state, and restart-safe asynchronous execution.', 'amber'],
  ['Observability', 'Expose real production counters, latency histograms, queue depth, and request metrics directly from the running system.', 'green'],
]

const HOW_IT_WORKS = [
  ['Async Processing', 'Repository analysis runs asynchronously through Redis Streams and a worker pool, so the browser is never the execution engine.'],
  ['Reliability', 'Retryable failures are bounded by configured retry attempts while non-retryable outcomes and exhausted retries are routed to a dead-letter stream.'],
  ['Persistence', 'Job lifecycle state and completed analysis are persisted independently of the frontend, allowing progress tracking beyond a single browser session.'],
  ['Observability', 'HTTP requests, job execution, retries, and worker activity are surfaced through metrics and trace-ready instrumentation.'],
]

const RELIABILITY_ITEMS = [
  'Redis-backed asynchronous jobs',
  'Worker pool processing',
  'Retry with bounded attempts',
  'Retryable vs non-retryable failure handling',
  'Dead-letter queue path',
  'Persistent job state',
  'Restart recovery behavior',
  'W3C trace propagation',
  'OpenTelemetry tracing hooks',
  'Prometheus metrics endpoint',
]

const LIVE_PIPELINE_STAGES = [
  ['Repository Submitted', 'Accepted by the Render API and validated before job orchestration.'],
  ['Job Created', 'A persistent job record is created and becomes addressable by job ID.'],
  ['Queued in Redis', 'The request is admitted into the Redis Stream backing the worker pipeline.'],
  ['Worker Processing', 'A live worker picks up the queued job for asynchronous execution.'],
  ['Repository Analysis', 'Static analysis, technology detection, health scoring, and insight generation run in the worker.'],
  ['Persistence', 'Results are persisted and become available through the analysis API.'],
  ['Analysis Complete', 'The browser fetches the completed analysis and renders the intelligence dashboard.'],
]

const ARCHITECTURE_FLOW = ['Browser', 'Vercel Frontend', 'Render API', 'MySQL Persistence', 'Redis Stream', 'Worker Pool', 'Repository Analysis', 'MySQL', 'Frontend']
const RELIABILITY_FLOW = ['Worker Failure', 'Retry', 'Retry Limit', 'Dead Letter Queue']
const OBSERVABILITY_FLOW = ['HTTP / Job', 'OpenTelemetry', 'Collector', 'Prometheus', 'Grafana', 'Jaeger']

const STATUS_COPY = {
  idle: 'Submit a repository to run the production analysis pipeline.',
  submitting: 'Submitting repository for analysis...',
  polling: 'Polling the live job status from the backend worker system...',
  done: 'Analysis complete.',
  error: 'The latest request did not complete successfully.',
}

function cn(...parts) {
  return parts.filter(Boolean).join(' ')
}

function validateRepositoryUrl(rawUrl) {
  const value = (rawUrl || '').trim()
  if (!value) return { isValid: false, message: 'Enter a repository URL to continue.' }

  let url
  try {
    url = new URL(value)
  } catch {
    return { isValid: false, message: 'Use a valid URL format, for example https://github.com/user/repo.' }
  }

  if (url.protocol !== 'https:' && url.protocol !== 'http:') {
    return { isValid: false, message: 'Repository URLs must start with http:// or https://.' }
  }

  const host = url.hostname.toLowerCase()
  const supported = SUPPORTED_REPO_HOST_PATTERNS.some((pattern) => host === pattern || host.endsWith(`.${pattern}`))
  if (!supported) {
    return { isValid: false, message: 'Supported hosts: GitHub, GitLab, Bitbucket, and Azure DevOps.' }
  }

  const parts = url.pathname.replace(/\.git$/, '').split('/').filter(Boolean)
  if (parts.length < 2) {
    return { isValid: false, message: 'Repository URLs must include both owner or group and repository name.' }
  }

  return { isValid: true, message: '' }
}

function parseRepoFromUrl(rawUrl) {
  if (!rawUrl) return null
  try {
    const url = new URL(rawUrl)
    const parts = url.pathname.replace(/\.git$/, '').split('/').filter(Boolean)
    if (parts.length >= 2) {
      return {
        owner: parts[0],
        name: parts[1],
        fullName: `${parts[0]}/${parts[1]}`,
        githubLink: `${url.protocol}//${url.host}/${parts[0]}/${parts[1]}`,
      }
    }
  } catch {
    return null
  }
  return null
}

function toLocale(value) {
  return typeof value === 'number' ? value.toLocaleString() : value || '0'
}

function formatNumber(value, fallback = 'Unavailable') {
  return typeof value === 'number' && Number.isFinite(value) ? value.toLocaleString() : fallback
}

function formatPercent(value, digits = 0) {
  if (typeof value !== 'number' || Number.isNaN(value)) return 'Unavailable'
  return `${value.toFixed(digits)}%`
}

function formatDate(value) {
  if (!value) return 'Unavailable'
  const date = new Date(value)
  return Number.isNaN(date.getTime()) ? 'Unavailable' : date.toLocaleString()
}

function durationBetween(startValue, endValue) {
  if (!startValue || !endValue) return null
  const start = new Date(startValue).getTime()
  const end = new Date(endValue).getTime()
  if (Number.isNaN(start) || Number.isNaN(end) || end < start) return null
  return end - start
}

function formatDuration(durationMs) {
  if (typeof durationMs !== 'number' || Number.isNaN(durationMs) || durationMs < 0) return 'Unavailable'
  if (durationMs < 1000) return `${Math.round(durationMs)} ms`
  const seconds = durationMs / 1000
  if (seconds < 60) return `${seconds.toFixed(seconds < 10 ? 1 : 0)} s`
  const minutes = Math.floor(seconds / 60)
  return `${minutes}m ${Math.round(seconds % 60)}s`
}

function resolveRepositoryIdentity(meta, submittedUrl) {
  const fromUrl = parseRepoFromUrl(submittedUrl)
  if (meta?.fullName) {
    return {
      label: meta.fullName,
      owner: meta.owner || meta.fullName.split('/')[0],
      name: meta.name || meta.fullName.split('/')[1],
      link: `https://github.com/${meta.fullName}`,
    }
  }
  if (meta?.owner && meta?.name) {
    return {
      label: `${meta.owner}/${meta.name}`,
      owner: meta.owner,
      name: meta.name,
      link: `https://github.com/${meta.owner}/${meta.name}`,
    }
  }
  if (fromUrl) {
    return {
      label: fromUrl.fullName,
      owner: fromUrl.owner,
      name: fromUrl.name,
      link: fromUrl.githubLink,
    }
  }
  return { label: 'Repository analysis', owner: 'Unknown', name: 'Repository', link: submittedUrl || null }
}

function parseLabels(rawLabels) {
  const labels = {}
  if (!rawLabels) return labels
  const regex = /(\w+)="([^"]*)"/g
  let match = regex.exec(rawLabels)
  while (match) {
    labels[match[1]] = match[2]
    match = regex.exec(rawLabels)
  }
  return labels
}

function parsePrometheusMetrics(text) {
  const lines = (text || '').split('\n').map((line) => line.trim()).filter(Boolean)
  const summary = { counters: {}, durationBuckets: [], routeCounts: [] }
  const routeMap = new Map()
  const bucketValues = []

  for (const line of lines) {
    if (line.startsWith('#')) continue
    const match = line.match(/^([a-zA-Z_:][a-zA-Z0-9_:]*)(\{([^}]*)\})?\s+([-+]?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?)$/)
    if (!match) continue
    const name = match[1]
    const labels = parseLabels(match[3])
    const value = Number.parseFloat(match[4])

    if (name === 'cortex_job_processing_duration_seconds_bucket') {
      bucketValues.push({ le: labels.le, value })
      continue
    }

    if (name === 'cortex_http_requests_total') {
      const key = `${labels.method || 'GET'} ${labels.route || '/'}`
      routeMap.set(key, { key, method: labels.method || 'GET', route: labels.route || '/', status: labels.status || '200', count: value })
      continue
    }

    summary.counters[name] = value
  }

  let previous = 0
  summary.durationBuckets = bucketValues
    .filter((bucket) => bucket.le && bucket.le !== '+Inf')
    .map((bucket) => {
      const delta = Math.max(0, bucket.value - previous)
      previous = bucket.value
      return { le: Number.parseFloat(bucket.le), cumulative: bucket.value, count: delta }
    })

  summary.routeCounts = Array.from(routeMap.values()).sort((a, b) => b.count - a.count)

  const jobsSubmitted = summary.counters.cortex_jobs_submitted_total || 0
  const jobsCompleted = summary.counters.cortex_jobs_completed_total || 0
  const jobsFailed = summary.counters.cortex_jobs_failed_total || 0
  const jobsRetried = summary.counters.cortex_jobs_retried_total || 0
  const jobsDeadLettered = summary.counters.cortex_jobs_dead_lettered_total || 0
  const jobsActive = summary.counters.cortex_jobs_active || 0
  const queueDepth = summary.counters.cortex_jobs_queue_depth || 0
  const durationCount = summary.counters.cortex_job_processing_duration_seconds_count || 0
  const durationSum = summary.counters.cortex_job_processing_duration_seconds_sum || 0

  summary.overview = {
    jobsSubmitted,
    jobsCompleted,
    jobsFailed,
    jobsRetried,
    jobsDeadLettered,
    jobsActive,
    queueDepth,
    averageProcessingSeconds: durationCount > 0 ? durationSum / durationCount : null,
    successRate: jobsSubmitted > 0 ? (jobsCompleted / jobsSubmitted) * 100 : null,
    failureRate: jobsSubmitted > 0 ? (jobsFailed / jobsSubmitted) * 100 : null,
  }

  return summary
}

function uniqTechItems(tech) {
  if (!tech) return []
  const seen = new Set()
  const items = []
  for (const [key] of TECH_GROUPS) {
    for (const item of tech[key] || []) {
      if (!item?.name) continue
      const token = item.name.toLowerCase()
      if (seen.has(token)) continue
      seen.add(token)
      items.push(item)
    }
  }
  return items
}

function buildLanguageRows(languages) {
  const entries = Object.entries(languages || {}).sort((a, b) => b[1] - a[1])
  const total = entries.reduce((sum, [, count]) => sum + count, 0)
  return entries.map(([ext, count]) => ({ ext, count, percent: total > 0 ? (count / total) * 100 : 0 }))
}

function getStatusTone(status) {
  const normalized = (status || '').toUpperCase()
  if (normalized === 'COMPLETED') return 'success'
  if (normalized === 'FAILED') return 'danger'
  if (normalized === 'RUNNING') return 'active'
  if (normalized === 'QUEUED') return 'pending'
  return 'neutral'
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

function deriveRiskSignals(analysis) {
  const health = analysis?.repositoryHealth
  const insights = analysis?.repositoryInsights
  return {
    warnings: (health?.warnings || []).length,
    risks: (insights?.risks || []).length,
  }
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
    { label: 'Submitted', timestamp: jobDetails.job.createdAt, note: 'Job record persisted by the API.' },
  ]

  if (jobDetails.queue?.streamId) {
    timeline.push({ label: 'Queued', timestamp: null, note: 'Stream record observed in the Redis-backed queue.' })
  }

  if (jobDetails.job.startedAt) {
    timeline.push({ label: 'Processing', timestamp: jobDetails.job.startedAt, note: 'Worker execution started.' })
  }

  if (jobDetails.job.status === 'COMPLETED') {
    timeline.push({ label: 'Analysis Complete', timestamp: jobDetails.job.completedAt, note: 'Analysis persisted and available through the API.' })
  }

  if (jobDetails.job.status === 'FAILED') {
    timeline.push({ label: 'Failure', timestamp: jobDetails.job.completedAt, note: jobDetails.reliability?.failureReason || 'Failure reason not persisted.' })
  }

  if (jobDetails.reliability?.deadLettered) {
    timeline.push({ label: 'Dead Letter', timestamp: null, note: 'The job has a dead-letter record in the reliability path.' })
  }

  return timeline
}

function getErrorMessage(message) {
  const text = (message || '').toLowerCase()
  if (!message) return 'The request could not be completed.'
  if (text.includes('busy')) return 'The queue is currently busy. The distributed worker pipeline rejected new work for the moment.'
  if (text.includes('timeout')) return 'The backend did not respond in time. The request may still be running on the server.'
  return message
}

function MicroLabel({ children }) {
  return <p className="micro-label">{children}</p>
}

function SectionHeading({ eyebrow, title, description, anchor }) {
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

function Hero({ url, setUrl, phase, validation, onSubmit, onUseExample }) {
  const showError = url.trim().length > 0 && !validation.isValid
  return (
    <section className="hero-panel">
      <div className="hero-copy">
        <div className="hero-badges" aria-label="Cortex platform highlights">
          <span className="eyebrow-pill">Distributed code intelligence</span>
          <span className="eyebrow-pill neutral">Vercel → Render → Redis → Worker Pool</span>
        </div>
        <h1>Understand any codebase in minutes.</h1>
        <p className="hero-summary">
          Cortex turns a public repository into a production-style engineering intelligence report using a live asynchronous backend, Redis Streams, worker processing, persistent job state, and GitHub metadata enrichment.
        </p>
        <form className="hero-form" onSubmit={onSubmit}>
          <label className="sr-only" htmlFor="repositoryUrl">Repository URL</label>
          <div className="hero-input-shell">
            <span className="input-prefix">repo</span>
            <input
              id="repositoryUrl"
              type="text"
              value={url}
              onChange={(event) => setUrl(event.target.value)}
              placeholder="https://github.com/user/repository"
              className={cn('hero-input', !validation.isValid && url.trim() ? 'is-invalid' : '')}
              disabled={phase === 'submitting' || phase === 'polling'}
              aria-invalid={showError}
            />
          </div>
          <button type="submit" className="hero-cta" disabled={phase === 'submitting' || phase === 'polling' || !validation.isValid}>
            {phase === 'submitting' || phase === 'polling' ? 'Analyzing…' : 'Analyze Repository'}
          </button>
        </form>
        {showError ? <p className="form-message error">{validation.message}</p> : null}
        <div className="hero-actions">
          <button type="button" className="secondary-button" onClick={onUseExample}>Try an example</button>
          <a className="text-link" href={PROJECT_LINKS.repo} target="_blank" rel="noopener noreferrer">Inspect the source architecture</a>
        </div>
        <div className="hero-meta-grid">
          <div>
            <MicroLabel>Quick start</MicroLabel>
            <p className="mono">{EXAMPLE_REPOSITORY}</p>
          </div>
          <div>
            <MicroLabel>Execution model</MicroLabel>
            <p>Submission is asynchronous and processed by a distributed worker system.</p>
          </div>
        </div>
      </div>
      <div className="hero-aside">
        <div className="hero-terminal">
          <div className="terminal-bar"><span /><span /><span /></div>
          <div className="terminal-body">
            <p><span>$</span> POST /repositories</p>
            <p><span>→</span> job persisted</p>
            <p><span>→</span> queued in Redis Stream</p>
            <p><span>→</span> worker pool picks up job</p>
            <p><span>→</span> analysis + GitHub enrichment</p>
            <p><span>→</span> results persisted and rendered</p>
          </div>
        </div>
        <div className="hero-side-card">
          <MicroLabel>What makes this demo different</MicroLabel>
          <ul>
            <li>Live job orchestration, not mocked analysis.</li>
            <li>Production metrics and latency histograms.</li>
            <li>Repository health, technology detection, and GitHub intelligence in one flow.</li>
          </ul>
        </div>
      </div>
    </section>
  )
}

function CapabilityGrid() {
  return (
    <section className="panel-grid capabilities-panel">
      {CAPABILITY_CARDS.map(([title, description, accent]) => (
        <article key={title} className={cn('capability-card', `accent-${accent}`)}>
          <MicroLabel>Showcase capability</MicroLabel>
          <h3>{title}</h3>
          <p>{description}</p>
        </article>
      ))}
    </section>
  )
}

function ResultsNav({ hasAnalysis }) {
  const items = [['pipeline', 'Live job'], ['system', 'System'], ['architecture', 'Pipeline'], ['reliability', 'Reliability'], ['how-it-works', 'How Cortex Works']]
  if (hasAnalysis) {
    items.push(['analysis-dashboard', 'Analysis'], ['repository-overview', 'Overview'], ['technology-stack', 'Technology'], ['language-intelligence', 'Languages'], ['repository-health', 'Health'], ['engineering-insights', 'Insights'], ['github-intelligence', 'GitHub'])
  }
  return (
    <nav className="results-nav" aria-label="Cortex sections">
      {items.map(([href, label]) => <a key={href} href={`#${href}`}>{label}</a>)}
    </nav>
  )
}

function ErrorPanel({ message }) {
  if (!message) return null
  return (
    <section className="feedback-panel error-panel" aria-live="polite">
      <div><MicroLabel>Request status</MicroLabel><h3>Analysis did not complete</h3></div>
      <p>{getErrorMessage(message)}</p>
    </section>
  )
}

function EmptyState() {
  return (
    <section className="feedback-panel empty-panel">
      <div><MicroLabel>Demo-ready workflow</MicroLabel><h3>Submit a repository to light up the full platform</h3></div>
      <p>Cortex uses the real production pipeline behind the scenes: API admission, Redis-backed queueing, worker execution, persistent results, and GitHub metadata enrichment.</p>
    </section>
  )
}

function LoadingSkeleton({ phase }) {
  return (
    <section className="feedback-panel loading-panel" aria-live="polite">
      <div><MicroLabel>Live analysis</MicroLabel><h3>{phase === 'submitting' ? 'Creating job and queueing work' : 'Worker pipeline is processing the repository'}</h3></div>
      <div className="skeleton-grid"><div className="skeleton-card" /><div className="skeleton-card" /><div className="skeleton-card" /><div className="skeleton-card" /></div>
    </section>
  )
}

function JobMetaCard({ title, value, tone = 'neutral' }) {
  return <div className={cn('meta-card', tone !== 'neutral' ? `tone-${tone}` : '')}><MicroLabel>{title}</MicroLabel><p>{value}</p></div>
}

function LivePipeline({ phase, job, analysis, statusMessage, systemMetrics, systemSummary, jobDetails, expanded, onToggle }) {
  const queueDepth = typeof systemSummary?.queue?.pending === 'number' && typeof systemSummary?.queue?.lag === 'number'
    ? systemSummary.queue.pending + systemSummary.queue.lag
    : systemMetrics?.overview?.queueDepth
  const workerState = jobDetails?.queue?.consumer
    ? jobDetails.queue.consumer
    : (typeof systemSummary?.workers?.active === 'number' ? `${systemSummary.workers.active}/${systemSummary.workers.configured || '?'} workers` : 'Unavailable')
  const durationMs = durationBetween(job?.createdAt, job?.completedAt) || durationBetween(job?.createdAt, job?.startedAt)
  return (
    <section id="pipeline" className="section-panel live-pipeline-section">
      <SectionHeading eyebrow="Live execution" title="Analysis pipeline" description="This view is driven by the real job lifecycle and production metrics exposed by the backend." />
      <div className="job-surface">
        <div className="job-summary-grid">
          <JobMetaCard title="Job ID" value={job?.jobId || 'Waiting for submission'} tone="cyan" />
          <JobMetaCard title="Current status" value={job?.status || 'Idle'} tone={getStatusTone(job?.status)} />
          <JobMetaCard title="Elapsed time" value={durationMs ? formatDuration(durationMs) : 'In progress'} tone="amber" />
          <JobMetaCard title="Queue depth" value={typeof queueDepth === 'number' ? formatNumber(queueDepth) : 'Unavailable'} />
          <JobMetaCard title="Worker state" value={workerState} />
          <JobMetaCard title="Attempt" value={typeof jobDetails?.reliability?.attempt === 'number' ? String(jobDetails.reliability.attempt) : 'Unavailable'} />
        </div>
        <div className="pipeline-track" aria-live="polite">
          {LIVE_PIPELINE_STAGES.map(([label, detail], index) => {
            const state = resolveLiveStageState(index, phase, job, analysis)
            return (
              <div key={label} className={cn('pipeline-step', `state-${state}`)}>
                <div className="pipeline-node"><span className="pipeline-index">{index + 1}</span></div>
                <div className="pipeline-copy"><p className="pipeline-label">{label}</p><p className="pipeline-detail">{detail}</p></div>
              </div>
            )
          })}
        </div>
        <div className="status-strip">
          <p>{statusMessage || STATUS_COPY[phase]}</p>
          {job?.jobId ? <button type="button" className="secondary-button subtle" onClick={onToggle}>{expanded ? 'Hide job detail' : 'Show job detail'}</button> : null}
        </div>
      </div>
    </section>
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
  return (
    <section className="section-panel job-detail-panel">
      <SectionHeading eyebrow="Job detail" title="Current job record" description="This panel now uses the enriched read-only job details endpoint for reliability, queue, and trace intelligence." />
      <div className="detail-grid">
        <div className="detail-stack">
          <div className="detail-row"><span>Job ID</span><strong className="mono">{detailJob.id || job.jobId}</strong></div>
          <div className="detail-row"><span>Repository</span><strong>{repo?.fullName || detailJob.repositoryUrl || 'Unavailable'}</strong></div>
          <div className="detail-row"><span>Status</span><strong>{detailJob.status || 'Unavailable'}</strong></div>
          <div className="detail-row"><span>Created</span><strong>{formatDate(detailJob.createdAt)}</strong></div>
          <div className="detail-row"><span>Started</span><strong>{formatDate(detailJob.startedAt)}</strong></div>
          <div className="detail-row"><span>Completed</span><strong>{formatDate(detailJob.completedAt)}</strong></div>
          <div className="detail-row"><span>Duration</span><strong>{duration ? formatDuration(duration) : 'In progress'}</strong></div>
          <div className="detail-row"><span>Attempt</span><strong>{typeof attempt === 'number' ? attempt : 'Unavailable'}</strong></div>
          <div className="detail-row"><span>Retry state</span><strong>{retryState}</strong></div>
          <div className="detail-row"><span>Dead-lettered</span><strong>{typeof deadLettered === 'boolean' ? (deadLettered ? 'Yes' : 'No') : 'Unavailable'}</strong></div>
          <div className="detail-row"><span>Failure reason</span><strong>{failureReason || 'Unavailable'}</strong></div>
          <div className="detail-row"><span>Retryable</span><strong>{typeof retryable === 'boolean' ? (retryable ? 'Yes' : 'No') : 'Unavailable'}</strong></div>
          <div className="detail-row"><span>Worker</span><strong>{jobDetails?.queue?.consumer || 'Unavailable'}</strong></div>
        </div>
        <div className="timeline-panel">
          {timeline.map((item) => <div className="timeline-item" key={`${item.label}-${item.timestamp || item.note}`}><div className="timeline-dot" /><div><p>{item.label}</p><span>{item.timestamp ? formatDate(item.timestamp) : item.note}</span></div></div>)}
          <div className="trace-card">
            <MicroLabel>Distributed trace</MicroLabel>
            <strong className="mono">{traceAvailable ? traceId : 'Unavailable'}</strong>
            <span>{traceAvailable ? 'Trace captured — available in observability backend' : 'Trace ID is not currently available for this job.'}</span>
          </div>
          <div className={cn('status-card', detailJob.status === 'FAILED' ? 'failed' : 'completed')}>
            <MicroLabel>{detailJob.status === 'FAILED' ? 'Failure state' : 'Completion state'}</MicroLabel>
            <h3>{detailJob.status}</h3>
            <p>{detailJob.status === 'FAILED' ? `Reason: ${failureReason || 'Unavailable'}` : `Duration: ${duration ? formatDuration(duration) : 'Unavailable'}`}</p>
            <p>Attempts: {typeof attempt === 'number' ? attempt : 'Unavailable'}</p>
            <p>Dead-lettered: {typeof deadLettered === 'boolean' ? (deadLettered ? 'Yes' : 'No') : 'Unavailable'}</p>
          </div>
        </div>
      </div>
    </section>
  )
}

function SystemOverview({ metrics, metricsState, systemSummary, systemState }) {
  const overview = metrics?.overview
  const queuePending = systemSummary?.queue?.pending
  const queueLag = systemSummary?.queue?.lag
  const jobs = systemSummary?.jobs
  const reliability = systemSummary?.reliability
  const workers = systemSummary?.workers
  const throughputItems = overview ? [
    { label: 'Submitted', value: jobs?.submitted ?? overview.jobsSubmitted },
    { label: 'Completed', value: jobs?.completed ?? overview.jobsCompleted },
    { label: 'Failed', value: jobs?.failed ?? overview.jobsFailed },
    { label: 'Retried', value: jobs?.retried ?? overview.jobsRetried },
    { label: 'Dead-lettered', value: jobs?.deadLettered ?? overview.jobsDeadLettered },
  ] : []
  const throughputMax = Math.max(...throughputItems.map((item) => item.value), 1)
  const latencyMax = Math.max(...(metrics?.durationBuckets || []).map((item) => item.count), 1)

  return (
    <section id="system" className="section-panel">
      <SectionHeading eyebrow="Production visibility" title="System and observability" description="These cards are backed by live metrics plus the read-only system summary endpoint." />
      {metricsState.error ? <p className="panel-note error">{metricsState.error}</p> : null}
      {systemState.error ? <p className="panel-note error">{systemState.error}</p> : null}
      <div className="stats-grid metrics-grid">
        <JobMetaCard title="Jobs submitted" value={jobs ? formatNumber(jobs.submitted) : overview ? formatNumber(overview.jobsSubmitted) : 'Loading…'} tone="cyan" />
        <JobMetaCard title="Jobs completed" value={jobs ? formatNumber(jobs.completed) : overview ? formatNumber(overview.jobsCompleted) : 'Loading…'} tone="success" />
        <JobMetaCard title="Jobs failed" value={jobs ? formatNumber(jobs.failed) : overview ? formatNumber(overview.jobsFailed) : 'Loading…'} tone="danger" />
        <JobMetaCard title="Jobs retried" value={jobs ? formatNumber(jobs.retried) : overview ? formatNumber(overview.jobsRetried) : 'Loading…'} tone="amber" />
        <JobMetaCard title="Dead-lettered" value={jobs ? formatNumber(jobs.deadLettered) : overview ? formatNumber(overview.jobsDeadLettered) : 'Loading…'} />
        <JobMetaCard title="Active jobs" value={jobs ? formatNumber(jobs.active) : overview ? formatNumber(overview.jobsActive) : 'Loading…'} />
        <JobMetaCard title="Queue pending" value={typeof queuePending === 'number' ? formatNumber(queuePending) : 'Unavailable'} />
        <JobMetaCard title="Queue lag" value={typeof queueLag === 'number' ? formatNumber(queueLag) : 'Unavailable'} />
        <JobMetaCard title="Avg processing time" value={overview?.averageProcessingSeconds != null ? `${overview.averageProcessingSeconds.toFixed(2)} s` : 'Unavailable'} />
        <JobMetaCard title="Success rate" value={typeof reliability?.successRate === 'number' ? formatPercent(reliability.successRate, 1) : overview?.successRate != null ? formatPercent(overview.successRate, 1) : 'Unavailable'} tone="success" />
        <JobMetaCard title="Failure rate" value={typeof reliability?.failureRate === 'number' ? formatPercent(reliability.failureRate, 1) : overview?.failureRate != null ? formatPercent(overview.failureRate, 1) : 'Unavailable'} tone="danger" />
        <JobMetaCard title="Retry rate" value={typeof reliability?.retryRate === 'number' ? formatPercent(reliability.retryRate, 1) : 'Unavailable'} tone="amber" />
        <JobMetaCard title="Worker count" value={workers ? `${workers.active ?? '?'} active / ${workers.configured ?? '?'} configured` : 'Unavailable'} />
      </div>
      <div className="dual-panel-grid">
        <div className="chart-panel">
          <div className="chart-header"><MicroLabel>Job throughput</MicroLabel><p>Lifetime counters since the current service boot.</p></div>
          <div className="bar-list">
            {throughputItems.map((item) => (
              <div key={item.label} className="bar-row">
                <span>{item.label}</span>
                <div className="bar-track"><div className="bar-fill" style={{ width: `${Math.max((item.value / throughputMax) * 100, 4)}%` }} /></div>
                <strong>{formatNumber(item.value)}</strong>
              </div>
            ))}
          </div>
        </div>
        <div className="chart-panel">
          <div className="chart-header"><MicroLabel>Processing latency</MicroLabel><p>Histogram derived from real worker timing buckets exposed by Prometheus format metrics.</p></div>
          <div className="latency-chart">
            {(metrics?.durationBuckets || []).map((bucket) => (
              <div key={bucket.le} className="latency-column">
                <div className="latency-bar" style={{ height: `${Math.max((bucket.count / latencyMax) * 100, 6)}%` }} />
                <span>{bucket.le}s</span>
              </div>
            ))}
          </div>
        </div>
      </div>
      <div className="chart-panel routes-panel">
        <div className="chart-header"><MicroLabel>Request activity</MicroLabel><p>HTTP counters prove the backend is serving live API traffic rather than demo-only content.</p></div>
        <div className="route-grid">
          {(metrics?.routeCounts || []).slice(0, 6).map((route) => (
            <div className="route-card" key={`${route.method}-${route.route}-${route.status}`}>
              <span className="route-method">{route.method}</span>
              <strong>{route.route}</strong>
              <span>Status {route.status}</span>
              <p>{formatNumber(route.count)} calls</p>
            </div>
          ))}
        </div>
      </div>
    </section>
  )
}

function ArchitectureSection() {
  return (
    <section id="architecture" className="section-panel architecture-panel">
      <SectionHeading eyebrow="Engineering pipeline" title="How the platform moves work" description="This section makes the distributed architecture understandable directly from the UI." />
      <div className="flow-block"><MicroLabel>Primary execution path</MicroLabel><div className="flow-line">{ARCHITECTURE_FLOW.map((step, index) => <div key={step} className="flow-step"><div className="flow-pill">{step}</div>{index < ARCHITECTURE_FLOW.length - 1 ? <div className="flow-arrow" aria-hidden="true">↓</div> : null}</div>)}</div></div>
      <div className="triple-flow-grid">
        <div className="mini-flow-card"><MicroLabel>Reliability path</MicroLabel>{RELIABILITY_FLOW.map((step, index) => <div key={step} className="mini-flow-item"><span>{step}</span>{index < RELIABILITY_FLOW.length - 1 ? <b>↓</b> : null}</div>)}</div>
        <div className="mini-flow-card"><MicroLabel>Observability path</MicroLabel>{OBSERVABILITY_FLOW.map((step, index) => <div key={step} className="mini-flow-item"><span>{step}</span>{index < OBSERVABILITY_FLOW.length - 1 ? <b>↓</b> : null}</div>)}</div>
        <div className="mini-flow-card"><MicroLabel>What the evaluator should see</MicroLabel><ul className="check-list compact"><li>Live backend processing</li><li>Persistent job state</li><li>Queue-backed worker orchestration</li><li>GitHub intelligence surfaced in-product</li></ul></div>
      </div>
    </section>
  )
}

function ReliabilitySection() {
  return (
    <section id="reliability" className="section-panel">
      <SectionHeading eyebrow="Reliability showcase" title="Operational capabilities already implemented" description="This is the engineering layer behind the UX, promoted as a first-class portfolio capability instead of hidden infrastructure." />
      <div className="reliability-grid">{RELIABILITY_ITEMS.map((item) => <div key={item} className="reliability-item"><span aria-hidden="true">✓</span><p>{item}</p></div>)}</div>
    </section>
  )
}

function HowItWorksSection() {
  return (
    <section id="how-it-works" className="section-panel">
      <SectionHeading eyebrow="Plain-English architecture" title="How Cortex works" description="A recruiter or interviewer should understand the backend design without opening the codebase." />
      <div className="explanation-grid">{HOW_IT_WORKS.map(([title, body]) => <article key={title} className="explanation-card"><MicroLabel>{title}</MicroLabel><p>{body}</p></article>)}</div>
    </section>
  )
}

function AnalysisSummaryCards({ analysis, job }) {
  const riskSignals = deriveRiskSignals(analysis)
  const duration = durationBetween(job?.createdAt, job?.completedAt)
  const languageCount = Object.keys(analysis.languages || {}).length
  const cards = [
    ['Files', toLocale(analysis.fileCount)],
    ['Directories', toLocale(analysis.dirCount)],
    ['Lines of code', toLocale(analysis.totalLines)],
    ['Languages', toLocale(languageCount)],
    ['Repository health', analysis.repositoryHealth ? `${analysis.repositoryHealth.overallScore} / 100` : 'Unavailable'],
    ['Complexity', analysis.repositoryInsights?.estimatedComplexity || 'Unavailable'],
    ['Risk indicators', `${riskSignals.risks} risks · ${riskSignals.warnings} warnings`],
    ['Analysis duration', duration ? formatDuration(duration) : 'Unavailable'],
  ]
  return <div className="stats-grid analysis-stats-grid">{cards.map(([label, value]) => <JobMetaCard key={label} title={label} value={value} />)}</div>
}

function RepositoryOverview({ analysis, submittedUrl }) {
  const meta = analysis.metadata || null
  const identity = resolveRepositoryIdentity(meta, submittedUrl)
  const rows = [
    ['Owner', meta?.owner || identity.owner || 'Unavailable'],
    ['Repository', meta?.name || identity.name || 'Unavailable'],
    ['Description', meta?.description || 'Unavailable'],
    ['Stars', typeof meta?.stars === 'number' ? toLocale(meta.stars) : 'Unavailable'],
    ['Forks', typeof meta?.forks === 'number' ? toLocale(meta.forks) : 'Unavailable'],
    ['Watchers', typeof meta?.watchers === 'number' ? toLocale(meta.watchers) : 'Unavailable'],
    ['Open issues', typeof meta?.openIssues === 'number' ? toLocale(meta.openIssues) : 'Unavailable'],
    ['Default branch', meta?.defaultBranch || 'Unavailable'],
    ['Last update', formatDate(meta?.updatedAt)],
    ['License', meta?.license || 'Unavailable'],
    ['Visibility', meta?.visibility || 'Unavailable'],
    ['Topics', (meta?.topics || []).length ? meta.topics.join(', ') : 'None'],
  ]
  return (
    <section id="repository-overview" className="section-panel">
      <SectionHeading eyebrow="Repository overview" title={identity.label} description="GitHub metadata, repository context, and analysis-derived identity signals are elevated to the top of the product surface." />
      {identity.link ? <a className="repo-link" href={identity.link} target="_blank" rel="noopener noreferrer">{identity.link}</a> : null}
      <div className="detail-grid wide">{rows.map(([label, value]) => <div className="detail-row card-row" key={label}><span>{label}</span><strong>{value}</strong></div>)}</div>
    </section>
  )
}

function TechnologyStack({ tech }) {
  if (!tech) return null
  const detected = uniqTechItems(tech)
  return (
    <section id="technology-stack" className="section-panel">
      <SectionHeading eyebrow="Architecture and stack" title="Technology stack" description={`Repository type: ${tech.repositoryType || 'Unknown'}${typeof tech.confidenceScore === 'number' ? ` · ${tech.confidenceScore}% confidence` : ''}`} />
      <div className="chip-showcase">{detected.length > 0 ? detected.map((item) => <span key={`${item.name}-${item.reason || ''}`} className="technology-chip" title={item.reason || ''}>{item.name}{typeof item.confidence === 'number' ? <b>{item.confidence}%</b> : null}</span>) : <p className="panel-note">No technology markers were detected for this repository.</p>}</div>
      <div className="tech-grid">{TECH_GROUPS.map(([key, label]) => <div className="tech-card" key={key}><MicroLabel>{label}</MicroLabel>{(tech[key] || []).length > 0 ? <ul className="text-list compact">{tech[key].map((item) => <li key={`${key}-${item.name}`}><strong>{item.name}</strong>{item.reason ? <span>{item.reason}</span> : null}</li>)}</ul> : <p className="panel-note">No detected signals.</p>}</div>)}</div>
    </section>
  )
}

function LanguageIntelligence({ analysis }) {
  const rows = useMemo(() => buildLanguageRows(analysis.languages), [analysis.languages])
  return (
    <section id="language-intelligence" className="section-panel">
      <SectionHeading eyebrow="Language intelligence" title="Language distribution" description="Visualize line distribution using real file-extension counts from the analysis result." />
      <div className="language-layout">
        <div className="language-bars">
          {rows.length > 0 ? rows.map((row) => (
            <div className="language-bar-row" key={row.ext}>
              <div className="language-bar-copy">
                <strong>{row.ext}</strong>
                <span>{formatNumber(row.count)} files</span>
              </div>
              <div className="language-bar-track">
                <div className="language-bar-fill" style={{ width: `${Math.max(row.percent, 1)}%` }} />
              </div>
              <strong>{formatPercent(row.percent, 0)}</strong>
            </div>
          )) : <p className="panel-note">No language distribution data available.</p>}
        </div>
        <div className="language-summary-card">
          <MicroLabel>Analysis footprint</MicroLabel>
          <div className="stat-stack">
            <div><span>Total lines</span><strong>{toLocale(analysis.totalLines)}</strong></div>
            <div><span>Files</span><strong>{toLocale(analysis.fileCount)}</strong></div>
            <div><span>Directories</span><strong>{toLocale(analysis.dirCount)}</strong></div>
            <div><span>Distinct languages</span><strong>{toLocale(rows.length)}</strong></div>
          </div>
        </div>
      </div>
    </section>
  )
}

function RepositoryHealth({ health }) {
  if (!health) return null
  const categories = [['documentation', 'Documentation'], ['testing', 'Testing'], ['ciCd', 'CI / CD'], ['security', 'Security'], ['maintainability', 'Maintainability'], ['configuration', 'Configuration'], ['projectStructure', 'Project structure']]
  return (
    <section id="repository-health" className="section-panel">
      <SectionHeading eyebrow="Engineering quality" title="Repository health" description="Health scoring combines documentation, testing, CI/CD, maintainability, configuration, security, and project structure." />
      <div className="health-hero"><div className="health-score-ring"><span>{health.overallScore}</span><small>/ 100</small></div><div><p className="grade-pill">Grade {health.grade}</p><p className="panel-note">Health warnings and recommendations are derived from static repository signals only.</p></div></div>
      <div className="health-bars">{categories.map(([key, label]) => { const item = health.categories?.[key]; if (!item) return null; const pct = item.maxScore > 0 ? (item.score / item.maxScore) * 100 : 0; return <div className="health-bar-row" key={key}><div className="health-bar-copy"><strong>{label}</strong><span>{item.score}/{item.maxScore} · {item.grade}</span></div><div className="health-bar-track"><div className="health-bar-fill" style={{ width: `${Math.max(pct, 2)}%` }} /></div></div> })}</div>
      <div className="triple-flow-grid"><div className="note-card"><MicroLabel>Strengths</MicroLabel><ul className="text-list">{(health.strengths || []).map((item) => <li key={item}>{item}</li>)}</ul></div><div className="note-card"><MicroLabel>Warnings</MicroLabel><ul className="text-list">{(health.warnings || []).map((item) => <li key={item}>{item}</li>)}</ul></div><div className="note-card"><MicroLabel>Recommendations</MicroLabel><ul className="text-list">{(health.recommendations || []).map((item) => <li key={item}>{item}</li>)}</ul></div></div>
    </section>
  )
}

function EngineeringInsights({ insights }) {
  if (!insights) return null
  return (
    <section id="engineering-insights" className="section-panel">
      <SectionHeading eyebrow="AI-style briefings" title="Engineering insights" description="Narrative output is derived from the analysis result and repository metadata already produced by the backend." />
      <div className="insight-callouts"><article className="insight-callout large"><MicroLabel>Summary</MicroLabel><p>{insights.summary || 'No summary available.'}</p></article><article className="insight-callout"><MicroLabel>Technology</MicroLabel><p>{insights.technologyOverview || 'No technology overview available.'}</p></article><article className="insight-callout"><MicroLabel>Quality</MicroLabel><p>{insights.qualityOverview || 'No quality overview available.'}</p></article></div>
      <div className="triple-flow-grid"><div className="note-card strong"><MicroLabel>Strengths</MicroLabel><ul className="text-list">{(insights.strengths || []).map((item) => <li key={item}>{item}</li>)}</ul></div><div className="note-card risk"><MicroLabel>Risks</MicroLabel><ul className="text-list">{(insights.risks || []).map((item) => <li key={item}>{item}</li>)}</ul></div><div className="note-card recommend"><MicroLabel>Recommendations</MicroLabel><ul className="text-list">{(insights.suggestions || []).map((item) => <li key={item}>{item}</li>)}</ul></div></div>
    </section>
  )
}

function GitHubIntelligence({ meta, submittedUrl }) {
  const repo = resolveRepositoryIdentity(meta, submittedUrl)
  return (
    <section id="github-intelligence" className="section-panel">
      <SectionHeading eyebrow="GitHub intelligence" title="Repository metadata" description="GitHub context is treated as a first-class intelligence source rather than a small sidebar block." />
      {meta ? <div className="github-grid"><a className="github-hero-card" href={repo.link || submittedUrl} target="_blank" rel="noopener noreferrer"><MicroLabel>Repository</MicroLabel><h3>{repo.label}</h3><p>{meta.description || 'No GitHub description available.'}</p></a><div className="github-metric-card"><span>Stars</span><strong>{toLocale(meta.stars)}</strong></div><div className="github-metric-card"><span>Forks</span><strong>{toLocale(meta.forks)}</strong></div><div className="github-metric-card"><span>Watchers</span><strong>{toLocale(meta.watchers)}</strong></div><div className="github-metric-card"><span>Open issues</span><strong>{toLocale(meta.openIssues)}</strong></div><div className="github-metric-card"><span>Default branch</span><strong>{meta.defaultBranch || 'Unavailable'}</strong></div><div className="github-metric-card"><span>License</span><strong>{meta.license || 'Unavailable'}</strong></div><div className="github-metric-card"><span>Last push</span><strong>{formatDate(meta.pushedAt)}</strong></div><div className="github-metric-card"><span>Topics</span><strong>{(meta.topics || []).length ? meta.topics.join(', ') : 'None'}</strong></div></div> : <div className="feedback-panel empty-panel small"><h3>GitHub metadata unavailable</h3><p>The backend returned a completed analysis without GitHub metadata for this run.</p></div>}
    </section>
  )
}

function AnalysisDashboard({ analysis, job, submittedUrl }) {
  return (
    <section id="analysis-dashboard" className="analysis-dashboard">
      <SectionHeading eyebrow="Completed analysis" title="Code intelligence dashboard" description="A production-grade repository intelligence view built from the current backend response shape." />
      <AnalysisSummaryCards analysis={analysis} job={job} />
      <RepositoryOverview analysis={analysis} submittedUrl={submittedUrl} />
      <TechnologyStack tech={analysis.technologyAnalysis} />
      <LanguageIntelligence analysis={analysis} />
      <RepositoryHealth health={analysis.repositoryHealth} />
      <EngineeringInsights insights={analysis.repositoryInsights} />
      <GitHubIntelligence meta={analysis.metadata} submittedUrl={submittedUrl} />
    </section>
  )
}

function Footer() {
  return (
    <footer className="footer-shell">
      <div className="footer-brand"><p className="footer-title">Cortex</p><p className="footer-copy">Distributed code intelligence for public repositories.</p></div>
      <div className="footer-links"><a href={PROJECT_LINKS.repo} target="_blank" rel="noopener noreferrer">Source</a><a href={PROJECT_LINKS.health} target="_blank" rel="noopener noreferrer">Health</a><a href={PROJECT_LINKS.metrics} target="_blank" rel="noopener noreferrer">Metrics</a><a href={PROFILE_LINKS.github} target="_blank" rel="noopener noreferrer">GitHub</a><a href={PROFILE_LINKS.linkedin} target="_blank" rel="noopener noreferrer">LinkedIn</a></div>
    </footer>
  )
}

export default function App() {
  const [url, setUrl] = useState(EXAMPLE_REPOSITORY)
  const [submittedUrl, setSubmittedUrl] = useState('')
  const [phase, setPhase] = useState('idle')
  const [statusMessage, setStatusMessage] = useState('')
  const [errorMessage, setErrorMessage] = useState('')
  const [analysis, setAnalysis] = useState(null)
  const [job, setJob] = useState(null)
  const [jobDetails, setJobDetails] = useState(null)
  const [metricsState, setMetricsState] = useState({ data: null, error: '', lastUpdated: '' })
  const [systemState, setSystemState] = useState({ data: null, error: '', lastUpdated: '' })
  const [jobDetailOpen, setJobDetailOpen] = useState(true)
  const pollTimeout = useRef(null)
  const urlValidation = useMemo(() => validateRepositoryUrl(url), [url])

  const refreshMetrics = useCallback(async () => {
    try {
      const response = await fetch(`${API}/metrics`)
      const text = await response.text()
      const parsed = parsePrometheusMetrics(text)
      setMetricsState({ data: parsed, error: '', lastUpdated: new Date().toISOString() })
    } catch (error) {
      setMetricsState((current) => ({ data: current.data, error: `Metrics unavailable: ${error.message}`, lastUpdated: current.lastUpdated }))
    }
  }, [])

  const refreshSystemSummary = useCallback(async () => {
    try {
      const response = await fetch(`${API}/system/summary`)
      const payload = await response.json()
      if (!payload.success) {
        throw new Error(payload.message || 'System summary unavailable')
      }
      setSystemState({ data: payload.data, error: '', lastUpdated: new Date().toISOString() })
    } catch (error) {
      setSystemState((current) => ({ data: current.data, error: `System summary unavailable: ${error.message}`, lastUpdated: current.lastUpdated }))
    }
  }, [])

  const refreshJobDetails = useCallback(async (jobId) => {
    if (!jobId) return
    try {
      const response = await fetch(`${API}/jobs/${jobId}/details`)
      const payload = await response.json()
      if (payload.success && payload.data) {
        setJobDetails(payload.data)
      }
    } catch {
      // Keep last known job details rather than replacing them with synthetic placeholders.
    }
  }, [])

  useEffect(() => {
    refreshMetrics()
    refreshSystemSummary()
    const timer = window.setInterval(() => {
      refreshMetrics()
      refreshSystemSummary()
    }, 15000)
    return () => window.clearInterval(timer)
  }, [refreshMetrics, refreshSystemSummary])

  useEffect(() => () => {
    if (pollTimeout.current) window.clearTimeout(pollTimeout.current)
  }, [])

  const finishWithAnalysis = useCallback(async (jobId) => {
    const response = await fetch(`${API}/analysis/${jobId}`)
    const payload = await response.json()
    if (!payload.success) {
      setPhase('error')
      setErrorMessage(payload.message || 'Analysis was not available after the job completed.')
      setStatusMessage('The job completed, but the analysis payload could not be loaded.')
      return
    }
    setAnalysis(payload.data)
    setPhase('done')
    setErrorMessage('')
    setStatusMessage('Analysis complete. The full dashboard is now rendering live backend data.')
    setJobDetailOpen(true)
    refreshMetrics()
    refreshSystemSummary()
    refreshJobDetails(jobId)
  }, [refreshJobDetails, refreshMetrics, refreshSystemSummary])

  const pollJob = useCallback(async (jobId) => {
    if (pollTimeout.current) window.clearTimeout(pollTimeout.current)
    const tick = async () => {
      try {
        const response = await fetch(`${API}/jobs/${jobId}`)
        const payload = await response.json()
        if (!payload.success || !payload.data) {
          setPhase('error')
          setErrorMessage(payload.message || 'Job state could not be loaded.')
          setStatusMessage('The frontend lost visibility into the active job.')
          return
        }
        setJob(payload.data)
        refreshJobDetails(jobId)
        const status = (payload.data.status || '').toUpperCase()
        if (status === 'COMPLETED') {
          await finishWithAnalysis(jobId)
          return
        }
        if (status === 'FAILED') {
          setPhase('error')
          setErrorMessage('The backend marked the analysis job as FAILED.')
          setStatusMessage('The worker pipeline ended the job in a failed state.')
          refreshMetrics()
          refreshSystemSummary()
          return
        }
        setPhase('polling')
        setErrorMessage('')
        setStatusMessage(`Live job state: ${status || 'UNKNOWN'}`)
        pollTimeout.current = window.setTimeout(tick, 2000)
      } catch (error) {
        setPhase('error')
        setErrorMessage(`Job polling failed: ${error.message}`)
        setStatusMessage('The browser could not continue polling the live job endpoint.')
      }
    }
    await tick()
  }, [finishWithAnalysis, refreshJobDetails, refreshMetrics, refreshSystemSummary])

  const handleSubmit = useCallback(async (event) => {
    event.preventDefault()
    const cleanUrl = url.trim()
    const validation = validateRepositoryUrl(cleanUrl)
    if (!validation.isValid) {
      setPhase('error')
      setErrorMessage(validation.message)
      setStatusMessage('Validation failed before the job was submitted.')
      return
    }
    setSubmittedUrl(cleanUrl)
    setPhase('submitting')
    setStatusMessage('Submitting repository for analysis...')
    setErrorMessage('')
    setAnalysis(null)
    setJob(null)
    setJobDetails(null)
    setJobDetailOpen(true)
    try {
      const response = await fetch(`${API}/repositories`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ repositoryUrl: cleanUrl }),
      })
      const payload = await response.json()
      if (!payload.success || !payload.data?.jobId) {
        setPhase('error')
        setErrorMessage(payload.message || 'The backend rejected the repository submission.')
        setStatusMessage('Submission was not accepted by the distributed job pipeline.')
        refreshMetrics()
        refreshSystemSummary()
        return
      }
      setJob({ jobId: payload.data.jobId, repositoryUrl: cleanUrl, status: payload.data.status || 'QUEUED', createdAt: payload.data.createdAt || new Date().toISOString(), startedAt: payload.data.startedAt, completedAt: payload.data.completedAt })
      setStatusMessage(`Job accepted. Job ID: ${payload.data.jobId}`)
      refreshMetrics()
      refreshSystemSummary()
      refreshJobDetails(payload.data.jobId)
      await pollJob(payload.data.jobId)
    } catch (error) {
      setPhase('error')
      setErrorMessage(`Request failed: ${error.message}`)
      setStatusMessage('The browser could not reach the analysis API.')
    }
  }, [pollJob, refreshJobDetails, refreshMetrics, refreshSystemSummary, url])

  const hasAnalysis = Boolean(analysis)

  return (
    <div className="app-shell">
      <main className="page-shell" role="main">
        <Hero url={url} setUrl={setUrl} phase={phase} validation={urlValidation} onSubmit={handleSubmit} onUseExample={() => setUrl(EXAMPLE_REPOSITORY)} />
        <ResultsNav hasAnalysis={hasAnalysis} />
        <CapabilityGrid />
        <LivePipeline phase={phase} job={job} analysis={analysis} statusMessage={statusMessage} systemMetrics={metricsState.data} systemSummary={systemState.data} jobDetails={jobDetails} expanded={jobDetailOpen} onToggle={() => setJobDetailOpen((current) => !current)} />
        <JobDetail job={job} jobDetails={jobDetails} submittedUrl={submittedUrl} expanded={jobDetailOpen} />
        {phase === 'error' ? <ErrorPanel message={errorMessage || statusMessage} /> : null}
        {(phase === 'submitting' || phase === 'polling') ? <LoadingSkeleton phase={phase} /> : null}
        {phase === 'idle' && !analysis ? <EmptyState /> : null}
        <SystemOverview metrics={metricsState.data} metricsState={metricsState} systemSummary={systemState.data} systemState={systemState} />
        <ArchitectureSection />
        <ReliabilitySection />
        <HowItWorksSection />
        {hasAnalysis ? <AnalysisDashboard analysis={analysis} job={job} submittedUrl={submittedUrl} /> : null}
      </main>
      <Footer />
    </div>
  )
}
