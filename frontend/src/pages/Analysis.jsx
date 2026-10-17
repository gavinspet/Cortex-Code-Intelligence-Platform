import { durationBetween, formatDuration, parseRepoFromUrl } from '../lib/format.js'
import { AnalysisReport } from '../components/AnalysisReport.jsx'
import { ErrorPanel, LoadingSkeleton, PageHeader, StatusBadge } from '../components/ui.jsx'

export function AnalysisPage({ jobId, analysisJob }) {
  const { job, phase, statusMessage, errorMessage, analysis, submittedUrl } = analysisJob
  const isCurrentJob = job?.jobId === jobId
  const repositoryUrl = submittedUrl || job?.repositoryUrl
  const repo = parseRepoFromUrl(repositoryUrl)
  const isLoading = !isCurrentJob || phase === 'submitting' || phase === 'polling'
  // Once the report is shown, the status badge says enough; progress messages are only useful while loading.
  const description = !isCurrentJob ? 'Loading job status...' : phase === 'done' ? null : statusMessage
  const duration = isCurrentJob ? durationBetween(job?.createdAt, job?.completedAt) : null

  return (
    <>
      <PageHeader
        eyebrow="Analysis report"
        title={isCurrentJob && repo ? repo.fullName : 'Repository analysis'}
        description={description}
      >
        <p className="page-meta">
          {isCurrentJob && job?.status ? <StatusBadge status={job.status} /> : null}
          <span>Job <span className="mono">{jobId}</span></span>
          {duration !== null ? <span>Duration {formatDuration(duration)}</span> : null}
          <a className="text-link" href="#/system">Processing details</a>
        </p>
      </PageHeader>
      {isCurrentJob && phase === 'error' ? <ErrorPanel message={errorMessage || statusMessage} /> : null}
      {isLoading ? <LoadingSkeleton phase={phase} /> : null}
      {isCurrentJob && analysis ? <AnalysisReport key={jobId} analysis={analysis} submittedUrl={repositoryUrl} /> : null}
    </>
  )
}
