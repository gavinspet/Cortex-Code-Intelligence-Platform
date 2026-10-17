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

export function parsePrometheusMetrics(text) {
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
