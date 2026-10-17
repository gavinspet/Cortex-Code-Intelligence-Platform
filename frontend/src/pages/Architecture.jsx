import { PageHeader, StatusBadge } from '../components/ui.jsx'
import {
  Arrow,
  Chain,
  ChipList,
  EnvTag,
  IconTile,
  InfoCard,
  Legend,
  SectionTitle,
  StageDiagram,
  StepList,
  TopologyDiagram,
} from '../components/ArchitectureVisuals.jsx'

// Every statement on this page reflects the backend source (backend/src) and
// docker-compose.yml. Keep it in sync when the backend changes.

const OVERVIEW_STAGES = [
  {
    title: 'Load',
    nodes: [
      { layer: 'frontend', icon: 'cloud', title: 'Vercel', tech: ['Vercel'], note: 'Serves the built app as static files.', tag: 'production' },
      { layer: 'frontend', icon: 'browser', title: 'Browser', tech: ['React', 'Vite'], note: 'Single-page app that calls the API directly.' },
    ],
  },
  {
    title: 'Submit',
    nodes: [
      { layer: 'api', icon: 'server', title: 'Render API', tech: ['C++', 'Drogon', 'Docker'], note: 'Validates the URL, creates the job, returns 202.', tag: 'production' },
    ],
  },
  {
    title: 'Queue',
    nodes: [
      { layer: 'data', icon: 'database', title: 'Job store', tech: ['MySQL'], note: 'Job saved as QUEUED. Falls back to memory.', tag: 'configurable' },
      { layer: 'queue', icon: 'queue', title: 'Redis Streams', tech: ['Redis'], note: 'Stream cortex:jobs, group cortex-workers.', tag: 'production' },
    ],
  },
  {
    title: 'Process',
    nodes: [
      { layer: 'queue', icon: 'workers', title: 'Worker pool', note: 'Four worker threads inside the API process.', tag: 'production' },
    ],
  },
  {
    title: 'Analyse',
    nodes: [
      { layer: 'analysis', icon: 'analysis', title: 'Repository analysis', tech: ['Git'], note: 'Clone, file scan, technology, health, insights.' },
      { layer: 'external', icon: 'globe', title: 'GitHub enrichment', tech: ['GitHub'], note: 'Repository metadata from the REST API.', tag: 'best-effort' },
    ],
  },
  {
    title: 'Results',
    nodes: [
      { layer: 'data', icon: 'layers', title: 'Stored results', note: 'Core counts in the job store; enrichment in memory.' },
      { layer: 'frontend', icon: 'browser', title: 'Report', note: 'The browser polls every 2 s, then renders the analysis.' },
    ],
  },
]

const TOPOLOGY = {
  clients: [
    { layer: 'frontend', icon: 'browser', title: 'Browser', tech: ['React', 'Vite'] },
    { layer: 'frontend', icon: 'cloud', title: 'Vercel', note: 'Static hosting with an SPA rewrite.' },
  ],
  clientLinks: [
    { from: 'Browser', to: 'Vercel', via: 'HTTPS · static files' },
    { from: 'Browser', to: 'API', via: 'HTTPS · JSON over CORS' },
  ],
  container: {
    title: 'Render · one Docker container',
    note: 'The HTTP API and the workers share one process.',
    tech: ['C++', 'Drogon', 'Docker'],
    nodes: [
      { layer: 'api', icon: 'server', title: 'HTTP API', note: 'Routes, CORS, URL validation' },
      { layer: 'queue', icon: 'workers', title: 'Worker pool', note: 'WORKER_COUNT threads, default 4' },
      { layer: 'analysis', icon: 'analysis', title: 'Analysis engine', note: 'Clones into /tmp/cortex-workspace' },
      { layer: 'data', icon: 'memory', title: 'In-memory stores', note: 'Metadata, technology, health, insights', tag: 'volatile' },
      { layer: 'observability', icon: 'chart', title: 'Telemetry', note: 'Prometheus metrics and OpenTelemetry spans' },
    ],
  },
  serviceLinks: [
    { from: 'API', to: 'Redis', via: 'XADD · XREADGROUP · XACK' },
    { from: 'API', to: 'MySQL', via: 'SQL · optional, via STORAGE_BACKEND' },
    { from: 'Workers', to: 'GitHub', via: 'git clone · REST API' },
  ],
  services: [
    { layer: 'queue', icon: 'queue', title: 'Redis', tech: ['Redis'], note: 'cortex:jobs and cortex:jobs:dead' },
    { layer: 'data', icon: 'database', title: 'MySQL', tech: ['MySQL'], note: 'jobs and analysis_results tables', tag: 'configurable' },
    { layer: 'external', icon: 'globe', title: 'GitHub', tech: ['GitHub'], note: 'Repository source and metadata API' },
  ],
}

const REQUEST_STEPS = [
  {
    layer: 'frontend',
    title: 'Submit',
    text: 'The browser sends the repository URL to the API.',
    code: 'POST /repositories',
  },
  {
    layer: 'api',
    title: 'Validate and create',
    text: 'Only HTTPS github.com and gitlab.com URLs are accepted; anything else returns 400. A UUID job is created with status QUEUED.',
    code: '400 Bad Request on invalid URL',
  },
  {
    layer: 'queue',
    title: 'Persist and enqueue',
    text: 'The job is saved, then added to the cortex:jobs stream with its attempt number and trace context. If the backlog has reached JOB_MAX_BACKLOG (default 64), the API returns 429.',
    code: '202 Accepted { jobId, status: "QUEUED" }',
  },
  {
    layer: 'queue',
    title: 'Claim',
    text: 'A worker first re-reads its own unacknowledged messages, then takes new ones. It marks the job RUNNING.',
    code: 'XREADGROUP GROUP cortex-workers',
  },
  {
    layer: 'analysis',
    title: 'Analyse',
    text: 'The worker clones and scans the repository, then runs enrichment. The job becomes COMPLETED and the message is acknowledged.',
    code: 'git clone --depth 1 · XACK',
  },
  {
    layer: 'frontend',
    title: 'Retrieve',
    text: 'The browser polls the job every 2 seconds and loads the merged analysis when it completes.',
    code: 'GET /jobs/{id} → GET /analysis/{id}',
  },
]

const WORKER_PIPELINE = [
  { layer: 'analysis', label: 'Clone', detail: 'Shallow, depth 1' },
  { layer: 'analysis', label: 'Scan files', detail: 'Files, folders, lines, extensions' },
  { layer: 'data', label: 'Save core result', detail: 'Job store' },
  { layer: 'external', label: 'GitHub metadata', detail: 'Skipped on failure' },
  { layer: 'analysis', label: 'Technology', detail: 'Frameworks and tooling' },
  { layer: 'analysis', label: 'Health', detail: 'Score and grade' },
  { layer: 'analysis', label: 'Insights', detail: 'Summary and findings' },
]

const TRANSITIONS = [
  { from: 'QUEUED', to: 'RUNNING', label: 'A worker claims the message', tone: 'info' },
  { from: 'RUNNING', to: 'COMPLETED', label: 'Every analysis step finishes', tone: 'success' },
  { from: 'RUNNING', to: 'QUEUED', label: 'Retryable failure, attempts remain', tone: 'warning' },
  { from: 'RUNNING', to: 'FAILED', label: 'Permanent failure or retries used up', tone: 'danger' },
]

const RELIABILITY_STEPS = [
  {
    layer: 'danger',
    icon: 'alert',
    title: 'Worker failure',
    text: 'A clone error, an exception during analysis, or a failed status update.',
  },
  {
    layer: 'warning',
    icon: 'route',
    title: 'Retry policy',
    text: 'Clone errors such as “repository not found” or “could not read Username” are permanent. Everything else is retryable.',
  },
  {
    layer: 'warning',
    icon: 'retry',
    title: 'Retry limit',
    text: 'The job returns to QUEUED and is re-added immediately with attempt + 1, up to JOB_MAX_RETRIES (default 3). There is no backoff delay.',
  },
  {
    layer: 'danger',
    icon: 'inbox',
    title: 'Dead-letter',
    text: 'The message is copied to cortex:jobs:dead, the job is marked FAILED, and the original message is acknowledged.',
  },
]

const DEAD_LETTER_OUTCOMES = [
  { icon: 'check', title: 'Job status', text: 'FAILED, with a completion time. GET /jobs/{id} reports it and the browser stops polling.' },
  { icon: 'inbox', title: 'Dead-letter entry', text: 'job_id, attempt, original_stream_id and a reason: max_retry_exceeded or clone_non_retryable_failure.' },
  { icon: 'chart', title: 'Metrics', text: 'cortex_jobs_failed_total and cortex_jobs_dead_lettered_total both increase.' },
  { icon: 'alert', title: 'No automatic reprocessing', text: 'Nothing consumes the dead-letter stream. It is a record for inspection.' },
]

const STORES = [
  {
    layer: 'data',
    icon: 'database',
    title: 'Job store',
    tag: 'configurable',
    where: 'MySQL when STORAGE_BACKEND=mysql and the connection succeeds; otherwise process memory.',
    items: [
      ['jobs', 'Status, timestamps, clone duration, error message'],
      ['analysis_results', 'File, folder and line counts; file-type distribution'],
    ],
    survives: 'Survives restarts with MySQL',
  },
  {
    layer: 'data',
    icon: 'memory',
    title: 'Process memory',
    tag: 'volatile',
    where: 'Always in memory, even when MySQL is enabled.',
    items: [
      ['metadata', 'GitHub stars, forks, licence, topics'],
      ['technology', 'Detected frameworks and tooling'],
      ['health', 'Score, grade and findings'],
      ['insights', 'Generated summary and suggestions'],
    ],
    survives: 'Cleared when the service restarts',
  },
  {
    layer: 'queue',
    icon: 'queue',
    title: 'Redis',
    where: 'Stream entries hold job references, not analysis results.',
    items: [
      ['cortex:jobs', 'Work queue, consumer group cortex-workers'],
      ['cortex:jobs:dead', 'Dead-letter record with a failure reason'],
    ],
    survives: 'Durability depends on the Redis deployment',
  },
]

const MERGE_SOURCES = [
  { label: 'analysis_results', where: 'Job store', layer: 'data' },
  { label: 'GitHub metadata', where: 'Memory', layer: 'data', volatile: true },
  { label: 'Technology', where: 'Memory', layer: 'data', volatile: true },
  { label: 'Health', where: 'Memory', layer: 'data', volatile: true },
  { label: 'Insights', where: 'Memory', layer: 'data', volatile: true },
]

const PRODUCTION_TELEMETRY = [
  {
    title: 'Metrics',
    items: [
      { layer: 'observability', label: 'Counters and histograms', detail: 'In process memory' },
      { layer: 'api', label: 'GET /metrics', detail: 'Prometheus text format', tag: 'production' },
    ],
  },
  {
    title: 'Status',
    items: [
      { layer: 'observability', label: 'Job and queue state', detail: 'Job store and Redis' },
      { layer: 'api', label: 'GET /system/summary', detail: 'And /jobs/{id}/details', tag: 'production' },
      { layer: 'frontend', label: 'System page', detail: 'This site' },
    ],
  },
  {
    title: 'Traces',
    items: [
      { layer: 'observability', label: 'OpenTelemetry spans', detail: 'W3C trace context' },
      { layer: 'observability', label: 'OTLP/HTTP exporter', detail: 'Batch processor' },
      { layer: 'external', label: 'Trace backend', detail: 'OTEL_EXPORTER_OTLP_TRACES_ENDPOINT', tag: 'unverified', dashed: true },
    ],
  },
]

const LOCAL_TELEMETRY = [
  {
    title: 'Traces',
    items: [
      { layer: 'api', label: 'Cortex API', detail: 'OTLP/HTTP' },
      { layer: 'observability', label: 'OTel Collector', detail: 'Port 4318' },
      { layer: 'observability', label: 'Jaeger UI', detail: 'Port 16686' },
    ],
  },
  {
    title: 'Metrics',
    items: [
      { layer: 'api', label: 'GET /metrics', detail: 'cortex:8080' },
      { layer: 'observability', label: 'Prometheus', detail: 'Scrapes every 15 s' },
      { layer: 'observability', label: 'Grafana', detail: 'Port 3001, provisioned dashboards' },
    ],
  },
]

const METRIC_FAMILIES = [
  'cortex_jobs_submitted_total',
  'cortex_jobs_completed_total',
  'cortex_jobs_failed_total',
  'cortex_jobs_retried_total',
  'cortex_jobs_dead_lettered_total',
  'cortex_jobs_active',
  'cortex_jobs_queue_depth',
  'cortex_job_processing_duration_seconds',
  'cortex_http_requests_total',
  'cortex_http_request_duration_seconds',
]

const SPAN_NAMES = [
  'repository.submit',
  'storage.job.persist',
  'redis.stream.xadd',
  'worker.consume_and_process',
  'job.process',
  'analysis.repository.clone',
  'analysis.static',
  'analysis.metadata',
  'analysis.technology',
  'analysis.health',
  'analysis.insight',
  'job.retry.schedule',
  'job.dead_letter.publish',
]

const CAPABILITIES = [
  { layer: 'queue', icon: 'queue', title: 'Asynchronous jobs', text: 'Redis Streams with a consumer group.' },
  { layer: 'queue', icon: 'workers', title: 'Worker pool', text: 'Thread count set by WORKER_COUNT, up to 16.' },
  { layer: 'api', icon: 'shield', title: 'Input validation', text: 'HTTPS GitHub and GitLab URLs only.' },
  { layer: 'api', icon: 'gauge', title: 'Backpressure', text: 'HTTP 429 when the backlog is full.' },
  { layer: 'warning', icon: 'retry', title: 'Bounded retries', text: 'Retryable and permanent failures handled separately.' },
  { layer: 'danger', icon: 'inbox', title: 'Dead-letter stream', text: 'Failed jobs recorded with a reason.' },
  { layer: 'queue', icon: 'route', title: 'Pending recovery', text: 'Unacknowledged messages resumed after a restart.' },
  { layer: 'data', icon: 'database', title: 'Optional MySQL', text: 'Jobs and core results, with in-memory fallback.' },
  { layer: 'observability', icon: 'chart', title: 'Prometheus metrics', text: 'Job, queue and HTTP metrics at /metrics.' },
  { layer: 'observability', icon: 'trace', title: 'Distributed tracing', text: 'OpenTelemetry spans across API, Redis and workers.' },
  { layer: 'external', icon: 'box', title: 'CI build', text: 'GitHub Actions compiles the backend on pushes and pull requests to main.' },
]

const NOT_IMPLEMENTED = ['Retry backoff', 'Job timeout', 'Idempotent submission', 'Dead-letter reprocessing', 'Pending hand-off between workers']

function TransitionGrid() {
  return (
    <ul className="arch-transitions" aria-label="Job status transitions">
      {TRANSITIONS.map((transition) => (
        <li key={`${transition.from}-${transition.to}`} className={`arch-transition arch-edge-${transition.tone}`}>
          <StatusBadge status={transition.from} />
          <span className="arch-transition-edge">
            <span className="arch-transition-label">{transition.label}</span>
            <span className="arch-transition-line" aria-hidden="true" />
            <span className="sr-only">, moves to</span>
          </span>
          <StatusBadge status={transition.to} />
        </li>
      ))}
    </ul>
  )
}

function ReliabilityFlow() {
  return (
    <div className="arch-reliability">
      <ol className="arch-reliability-flow" aria-label="Failure handling path">
        {RELIABILITY_STEPS.map((step, index) => (
          <li key={step.title} className={`arch-reliability-step layer-${step.layer}`}>
            <div className="arch-reliability-head">
              <IconTile layer={step.layer} icon={step.icon} />
              <h3><span className="sr-only">Step {index + 1}: </span>{step.title}</h3>
            </div>
            <p>{step.text}</p>
            {index < RELIABILITY_STEPS.length - 1 ? <Arrow className="arch-reliability-arrow" /> : null}
          </li>
        ))}
      </ol>
      <div className="arch-bypass">
        <span className="arch-bypass-bracket" aria-hidden="true" />
        <p className="arch-bypass-label">Permanent failures skip retries and go straight to the dead-letter stream.</p>
      </div>
    </div>
  )
}

function StoreCard({ store }) {
  return (
    <InfoCard layer={store.layer} icon={store.icon} title={store.title} tag={store.tag} element="li">
      <p className="arch-card-text">{store.where}</p>
      <dl className="arch-store-items">
        {store.items.map(([name, text]) => (
          <div key={name}>
            <dt><code>{name}</code></dt>
            <dd>{text}</dd>
          </div>
        ))}
      </dl>
      <p className="arch-store-survives">{store.survives}</p>
    </InfoCard>
  )
}

function MergeDiagram() {
  return (
    <div className="arch-merge">
      <ul className="arch-merge-sources" aria-label="Data combined into one analysis response">
        {MERGE_SOURCES.map((source) => (
          <li key={source.label} className={`arch-merge-source layer-${source.layer}${source.volatile ? ' is-dashed' : ''}`}>
            <span>{source.label}</span>
            <span className="arch-merge-where">{source.where}</span>
          </li>
        ))}
      </ul>
      <Arrow className="arch-merge-arrow" />
      <div className="arch-merge-target layer-api">
        <IconTile layer="api" icon="server" />
        <code>GET /analysis/{'{id}'}</code>
        <span>Combines all five into one response</span>
      </div>
      <Arrow className="arch-merge-arrow" />
      <div className="arch-merge-target layer-frontend">
        <IconTile layer="frontend" icon="browser" />
        <strong>Analysis report</strong>
        <span>Sections without data are hidden</span>
      </div>
    </div>
  )
}

function TelemetryLane({ id, title, tag, rows, local = false }) {
  return (
    <section className={`arch-lane${local ? ' is-local' : ''}`} aria-labelledby={id}>
      <div className="arch-lane-head">
        <h3 id={id}>{title}</h3>
        <EnvTag kind={tag} />
      </div>
      {rows.map((row) => (
        <div key={row.title} className="arch-lane-row">
          <p className="arch-lane-label">{row.title}</p>
          <Chain label={`${title}: ${row.title}`} items={row.items} />
        </div>
      ))}
    </section>
  )
}

export function ArchitecturePage() {
  return (
    <>
      <PageHeader
        eyebrow="How it works"
        title="Technical Architecture"
        description="Cortex queues each repository on Redis Streams and analyses it on background workers. The API answers immediately, and the browser follows the job until results are ready."
      />
      <article className="arch-page">
        <Legend />

        <section className="arch-section" aria-labelledby="arch-overview">
          <SectionTitle id="arch-overview" layer="frontend" icon="layers" title="System overview">
            One repository submission from start to finish. The numbered stages match the request flow below.
          </SectionTitle>
          <StageDiagram label="End-to-end request path" stages={OVERVIEW_STAGES} />
        </section>

        <section className="arch-section" aria-labelledby="arch-topology">
          <SectionTitle id="arch-topology" layer="api" icon="box" title="Deployment topology">
            Vercel only hosts the static app. The browser calls the Render API directly, and one container runs both the HTTP API and the worker pool.
          </SectionTitle>
          <TopologyDiagram {...TOPOLOGY} />
        </section>

        <section className="arch-section" aria-labelledby="arch-request-flow">
          <SectionTitle id="arch-request-flow" layer="queue" icon="route" title="Request flow">
            The API responds as soon as the job is queued. Analysis happens afterwards on a worker.
          </SectionTitle>
          <StepList steps={REQUEST_STEPS} />
          <div className="arch-subsection">
            <h3>Inside a worker</h3>
            <Chain label="Worker analysis steps" items={WORKER_PIPELINE} />
          </div>
        </section>

        <section className="arch-section" aria-labelledby="arch-lifecycle">
          <SectionTitle id="arch-lifecycle" layer="frontend" icon="check" title="Job lifecycle">
            Four statuses. COMPLETED and FAILED are final; a retry sends a job back to QUEUED.
          </SectionTitle>
          <TransitionGrid />
        </section>

        <section className="arch-section" aria-labelledby="arch-reliability">
          <SectionTitle id="arch-reliability" layer="danger" icon="shield" title="Reliability">
            Failures are classified first. Retryable failures are re-queued a limited number of times; everything else ends in the dead-letter stream.
          </SectionTitle>
          <ReliabilityFlow />
          <div className="arch-subsection">
            <h3>After retries are exhausted</h3>
            <ul className="arch-card-grid arch-card-grid-2">
              {DEAD_LETTER_OUTCOMES.map((outcome) => (
                <InfoCard key={outcome.title} layer="danger" icon={outcome.icon} title={outcome.title} element="li">
                  <p className="arch-card-text">{outcome.text}</p>
                </InfoCard>
              ))}
            </ul>
          </div>
          <ul className="arch-card-grid arch-card-grid-2">
            <InfoCard layer="api" icon="gauge" title="Backpressure" element="li">
              <p className="arch-card-text">Before queuing, the API adds up pending and undelivered messages. At JOB_MAX_BACKLOG (default 64) it returns 429 instead of accepting more work.</p>
            </InfoCard>
            <InfoCard layer="queue" icon="route" title="Restart recovery" element="li">
              <p className="arch-card-text">On start, each worker re-reads its own unacknowledged messages and resumes RUNNING jobs. The job record must still exist, so this relies on the MySQL job store.</p>
            </InfoCard>
          </ul>
        </section>

        <section className="arch-section" aria-labelledby="arch-persistence">
          <SectionTitle id="arch-persistence" layer="data" icon="database" title="Persistence">
            Results are not all stored the same way. Only jobs and core analysis results can be stored in MySQL; enrichment results always live in memory.
          </SectionTitle>
          <ul className="arch-card-grid arch-card-grid-3">
            {STORES.map((store) => <StoreCard key={store.title} store={store} />)}
          </ul>
          <div className="arch-subsection">
            <h3>How a report is assembled</h3>
            <MergeDiagram />
            <p className="arch-footnote">After a restart, older reports keep their file counts (with MySQL) but lose metadata, technology, health and insights. The production storage backend is set per deployment and is not reported by the API.</p>
          </div>
        </section>

        <section className="arch-section" aria-labelledby="arch-observability">
          <SectionTitle id="arch-observability" layer="observability" icon="chart" title="Observability">
            Metrics and status endpoints are live in production. The collector, Jaeger, the Prometheus server and Grafana run only in the local Docker Compose stack.
          </SectionTitle>
          <div className="arch-lanes">
            <TelemetryLane id="arch-lane-production" title="Production (Render)" tag="production" rows={PRODUCTION_TELEMETRY} />
            <TelemetryLane id="arch-lane-local" title="Local Docker Compose" tag="local" rows={LOCAL_TELEMETRY} local />
          </div>
          <p className="arch-footnote">Production job details include a trace ID, and the trace context travels inside each Redis message. No production trace backend is configured in this repository, so this page does not claim production traces are viewable. Metric counters reset when the service restarts.</p>
          <div className="arch-chip-columns">
            <div className="arch-subsection">
              <h3>Metric families</h3>
              <ChipList label="Prometheus metric families" items={METRIC_FAMILIES} mono />
            </div>
            <div className="arch-subsection">
              <h3>Trace spans</h3>
              <ChipList label="OpenTelemetry span names" items={SPAN_NAMES} mono />
            </div>
          </div>
        </section>

        <section className="arch-section" aria-labelledby="arch-capabilities">
          <SectionTitle id="arch-capabilities" layer="api" icon="check" title="Implemented capabilities" />
          <ul className="arch-card-grid arch-card-grid-auto">
            {CAPABILITIES.map((capability) => (
              <InfoCard key={capability.title} layer={capability.layer} icon={capability.icon} title={capability.title} element="li">
                <p className="arch-card-text">{capability.text}</p>
              </InfoCard>
            ))}
          </ul>
          <div className="arch-subsection">
            <h3>Not implemented</h3>
            <ChipList label="Not implemented" items={NOT_IMPLEMENTED} muted />
          </div>
        </section>
      </article>
    </>
  )
}
