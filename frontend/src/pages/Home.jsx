import { parseRepoFromUrl } from '../lib/format.js'
import { PROJECT_LINKS } from '../lib/links.js'
import { cn, ErrorPanel, LoadingSkeleton, StatusBadge } from '../components/ui.jsx'
import { LayerIcon, TechMarks } from '../components/ArchitectureVisuals.jsx'
import { BrandLogo } from '../components/BrandIcons.jsx'

// The example (and default input value) is Cortex's own repository.
export const EXAMPLE_REPOSITORY = PROJECT_LINKS.repo
const EXAMPLE_NAME = parseRepoFromUrl(EXAMPLE_REPOSITORY)?.fullName || EXAMPLE_REPOSITORY

const HOW_IT_WORKS = [
  { layer: 'frontend', icon: 'browser', label: 'Submit', note: 'Paste a public repository URL' },
  { layer: 'queue', icon: 'queue', label: 'Queue', note: 'The job joins a Redis stream' },
  { layer: 'analysis', icon: 'analysis', label: 'Analyse', note: 'A worker clones and inspects it' },
  { layer: 'data', icon: 'layers', label: 'Report', note: 'Structure, technology and health' },
]

const REPORT_CONTENTS = [
  { layer: 'frontend', icon: 'layers', art: 'bars', title: 'Structure', body: 'File, directory and line counts, and the number of files per extension.' },
  { layer: 'queue', icon: 'box', art: 'chips', title: 'Technology', body: 'Detected frameworks, build tools, package managers, testing, CI and container setup.' },
  { layer: 'data', icon: 'gauge', art: 'ring', title: 'Health and insights', body: 'A rule-based health score with strengths, risks and suggested improvements.' },
]

const STACK = ['C++', 'Drogon', 'Redis', 'MySQL', 'Docker', 'React', 'Vite']

function HowItWorks() {
  return (
    <ol className="home-flow" aria-label="How Cortex processes a repository">
      {HOW_IT_WORKS.map((step, index) => (
        <li key={step.label} className={cn('home-flow-step', `layer-${step.layer}`)} style={{ '--i': index }}>
          <span className="home-flow-icon"><LayerIcon name={step.icon} size={20} /></span>
          <span className="home-flow-text">
            <strong>{step.label}</strong>
            <span>{step.note}</span>
          </span>
          {index < HOW_IT_WORKS.length - 1 ? <span className="home-flow-link" aria-hidden="true"><span /></span> : null}
        </li>
      ))}
    </ol>
  )
}

// Abstract, unlabelled illustrations: they suggest the kind of section, not real values.
function FeatureArt({ kind }) {
  if (kind === 'bars') {
    return <span className="home-art home-art-bars" aria-hidden="true">{[55, 85, 40, 100, 65].map((h, i) => <span key={i} style={{ '--h': `${h}%`, '--i': i }} />)}</span>
  }
  if (kind === 'chips') {
    return <span className="home-art home-art-chips" aria-hidden="true">{[56, 38, 70, 46, 30, 60].map((w, i) => <span key={i} style={{ '--w': `${w}px`, '--i': i }} />)}</span>
  }
  return (
    <span className="home-art home-art-ring" aria-hidden="true">
      <svg viewBox="0 0 44 44"><circle cx="22" cy="22" r="17" /><circle cx="22" cy="22" r="17" pathLength="100" /></svg>
    </span>
  )
}

function AnalyzeForm({ url, setUrl, phase, validation, onSubmit }) {
  const busy = phase === 'submitting' || phase === 'polling'
  const showError = url.trim().length > 0 && !validation.isValid
  return (
    <form className="analyze-form" onSubmit={onSubmit} noValidate>
      <label className="field-label" htmlFor="repositoryUrl">Repository URL</label>
      <div className="analyze-row">
        <input
          id="repositoryUrl"
          type="text"
          inputMode="url"
          autoComplete="off"
          spellCheck="false"
          value={url}
          onChange={(event) => setUrl(event.target.value)}
          placeholder="https://github.com/owner/repository"
          className={cn('text-input', showError ? 'is-invalid' : '')}
          disabled={busy}
          aria-invalid={showError}
          aria-describedby={showError ? 'repositoryUrl-error repositoryUrl-hint' : 'repositoryUrl-hint'}
        />
        <button type="submit" className="button-primary" disabled={busy || !validation.isValid}>
          {busy ? 'Analyzing…' : 'Analyze Repository'}
        </button>
      </div>
      {showError ? <p id="repositoryUrl-error" className="form-message error">{validation.message}</p> : null}
      <p id="repositoryUrl-hint" className="form-hint form-hint-row">
        <span className="form-hint-hosts">
          <BrandLogo id="github" />
          <BrandLogo id="gitlab" />
          Public GitHub or GitLab repositories
        </span>
        <button
          type="button"
          className="example-chip"
          onClick={() => setUrl(EXAMPLE_REPOSITORY)}
          disabled={busy}
          title={EXAMPLE_REPOSITORY}
          aria-label={`Use example repository ${EXAMPLE_REPOSITORY}`}
        >
          <span className="example-chip-label">Try example</span>
          <span className="example-chip-repo">{EXAMPLE_NAME}</span>
        </button>
      </p>
    </form>
  )
}

function CurrentAnalysisLink({ job, submittedUrl }) {
  if (!job?.jobId) return null
  const repo = parseRepoFromUrl(submittedUrl || job.repositoryUrl)
  return (
    <section className="notice" aria-label="Current analysis">
      <span>Current analysis: <strong>{repo?.fullName || job.jobId}</strong></span>
      <StatusBadge status={job.status} />
      <a className="text-link" href={`#/analysis/${job.jobId}`}>View report</a>
    </section>
  )
}

export function HomePage({ url, setUrl, validation, onSubmit, analysisJob }) {
  const { phase, job, submittedUrl, errorMessage, statusMessage } = analysisJob
  // Errors that occur before a job is accepted (validation, 400, 429, network) are shown here;
  // errors for an accepted job are shown on its analysis page.
  const submissionFailed = phase === 'error' && !job?.jobId
  return (
    <>
      <section className="home-hero" aria-labelledby="home-title">
        <span className="page-header-orb" aria-hidden="true" />
        <span className="page-header-orb page-header-orb-2" aria-hidden="true" />
        <div className="home-hero-copy">
          <p className="page-header-eyebrow">Code intelligence</p>
          <h1 id="home-title">Analyze a public code repository</h1>
          <p className="home-lead">
            Enter a repository URL. Cortex clones the repository, inspects its contents and produces a report on its
            structure, technologies and engineering health.
          </p>
          <div className="home-form-card">
            <AnalyzeForm url={url} setUrl={setUrl} phase={phase} validation={validation} onSubmit={onSubmit} />
          </div>
        </div>
        <div className="home-hero-flow">
          <p className="home-hero-flow-title">How it works</p>
          <HowItWorks />
        </div>
      </section>
      {submissionFailed ? <ErrorPanel message={errorMessage || statusMessage} /> : null}
      {phase === 'submitting' ? <LoadingSkeleton phase={phase} /> : null}
      <CurrentAnalysisLink job={job} submittedUrl={submittedUrl} />
      <section className="home-contents" aria-labelledby="report-contents-heading">
        <h2 id="report-contents-heading">What the report includes</h2>
        <ul className="home-features">
          {REPORT_CONTENTS.map((item) => (
            <li key={item.title} className={cn('home-feature', `layer-${item.layer}`)}>
              <div className="home-feature-head">
                <span className="arch-icon-tile"><LayerIcon name={item.icon} /></span>
                <FeatureArt kind={item.art} />
              </div>
              <h3>{item.title}</h3>
              <p>{item.body}</p>
            </li>
          ))}
        </ul>
      </section>
      <section className="home-stack" aria-labelledby="home-stack-heading">
        <div>
          <h2 id="home-stack-heading">Under the hood</h2>
          <p>A C++ API with a Redis-backed worker pool, behind a React front end.</p>
        </div>
        <TechMarks names={STACK} />
        <a className="home-stack-link" href="#/architecture">See the architecture <span aria-hidden="true">→</span></a>
      </section>
    </>
  )
}
