import { useMemo, useState, useCallback } from 'react'

const getApiUrl = () => {
  const e = import.meta.env.VITE_API_URL
  if (e) return e
  if (typeof window !== 'undefined' && window.location.hostname.includes('vercel.app')) {
    return 'https://cortex-code-intelligence-platform.onrender.com'
  }
  return 'http://localhost:8080'
}

const API = getApiUrl()

const PROFILE_LINKS = {
  github: 'https://github.com/gavinspet',
  linkedin: 'https://linkedin.com',
  resume: 'https://github.com/gavinspet/Cortex-Code-Intelligence-Platform/blob/main/SPRINT_V2_ROADMAP.md',
}

const PROJECT_LINKS = {
  repo: 'https://github.com/gavinspet/Cortex-Code-Intelligence-Platform',
  health: 'https://cortex-code-intelligence-platform.onrender.com/health',
}

const TECH_GROUPS = [
  ['frontendFrameworks', 'Frontend'],
  ['backendFrameworks', 'Backend'],
  ['buildSystems', 'Build'],
  ['packageManagers', 'Package Manager'],
  ['testingFrameworks', 'Testing'],
  ['ciSystems', 'CI/CD'],
  ['containers', 'Containers'],
  ['cloudProviders', 'Cloud'],
  ['databases', 'Databases'],
]

const ASSET_EXTS = new Set(['.png', '.jpg', '.jpeg', '.gif', '.svg', '.ico', '.mp3', '.mp4', '.wav', '.webp', '.woff', '.woff2', '.ttf', '.otf', '.eot'])
const DOC_EXTS = new Set(['.md', '.rst', '.txt', '.adoc'])
const CONFIG_EXTS = new Set(['.json', '.yaml', '.yml', '.toml', '.xml', '.ini', '.env', '.cfg', '.conf'])

const LANG_COLORS = {
  '.js': '#f7df1e', '.jsx': '#61dafb', '.ts': '#3178c6', '.tsx': '#61dafb',
  '.py': '#3572a5', '.cpp': '#f34b7d', '.cc': '#f34b7d', '.cxx': '#f34b7d',
  '.c': '#9ca3af', '.h': '#d4a373', '.hpp': '#fb7185', '.java': '#f59e0b',
  '.go': '#22d3ee', '.rs': '#f97316', '.rb': '#ef4444', '.php': '#818cf8',
  '.cs': '#84cc16', '.swift': '#fb923c', '.kt': '#a78bfa', '.vue': '#34d399',
  '.svelte': '#f97316', '.html': '#fb923c', '.css': '#38bdf8', '.scss': '#f472b6',
  '.md': '#94a3b8', '.json': '#eab308', '.yml': '#f97316', '.yaml': '#f97316',
  '.sh': '#10b981', '.sql': '#f59e0b', '.xml': '#0ea5e9', '.toml': '#f59e0b',
}

const toLocale = (v) => (typeof v === 'number' ? v.toLocaleString() : v || '0')
const getLangColor = (ext) => LANG_COLORS[ext] || '#7c3aed'

const gradeClass = (grade) => {
  if (grade === 'A' || grade === 'B') return 'is-good'
  if (grade === 'C') return 'is-warn'
  return 'is-risk'
}

const healthTone = (pct) => {
  if (pct >= 80) return 'is-good'
  if (pct >= 60) return 'is-warn'
  return 'is-risk'
}

function languageGroup(ext) {
  if (ASSET_EXTS.has(ext)) return 'assets'
  if (DOC_EXTS.has(ext)) return 'docs'
  if (CONFIG_EXTS.has(ext)) return 'config'
  return 'code'
}

function MicroLabel({ children }) {
  return <span className="micro-label">{children}</span>
}

function DevHeader() {
  return (
    <header className="dev-header">
      <div className="dev-header-main">
        <p className="dev-name">KARTICK KUMAR GHOSH</p>
        <p className="dev-role">Software Developer · C++ Backend and Systems Engineer</p>
        <p className="dev-specialties mono">C++20 · Linux · Distributed Systems · REST APIs · System Design</p>
      </div>
      <nav className="dev-links" aria-label="Developer links">
        <a href={PROFILE_LINKS.github} target="_blank" rel="noopener noreferrer">GitHub</a>
        <a href={PROFILE_LINKS.linkedin} target="_blank" rel="noopener noreferrer">LinkedIn</a>
        <a href={PROFILE_LINKS.resume} target="_blank" rel="noopener noreferrer">Resume</a>
      </nav>
    </header>
  )
}

function Hero({ url, setUrl, phase, onSubmit }) {
  return (
    <section className="hero" aria-labelledby="cortex-title">
      <div className="hero-title-row">
        <h1 id="cortex-title">CORTEX</h1>
        <div className="hero-live mono">
          <span className="live-dot" /> LIVE / v1.x
        </div>
      </div>
      <p className="hero-subtitle">CODE INTELLIGENCE PLATFORM</p>
      <p className="hero-description">Analyze any public GitHub repository.</p>

      <form className="command-form" onSubmit={onSubmit}>
        <label className="sr-only" htmlFor="repositoryUrl">Repository URL</label>
        <input
          id="repositoryUrl"
          type="text"
          value={url}
          onChange={(e) => setUrl(e.target.value)}
          placeholder="https://github.com/user/repository"
          className="command-input mono"
          disabled={phase === 'submitting' || phase === 'polling'}
        />
        <button
          type="submit"
          className="command-button"
          disabled={phase === 'submitting' || phase === 'polling' || !url.trim()}
        >
          {phase === 'submitting' || phase === 'polling' ? 'ANALYZING' : 'ANALYZE'}
        </button>
      </form>

      <p className="hero-caption mono">Static analysis · Repository intelligence · Engineering insights</p>
    </section>
  )
}

function AnalysisJobStatus({ phase, statusMsg, jobStatus }) {
  if (!statusMsg) return null

  const s = (jobStatus || '').toUpperCase()
  const rows = [
    { label: 'JOB QUEUED', state: s === 'QUEUED' ? 'active' : (s === 'RUNNING' || s === 'COMPLETED') ? 'done' : 'pending' },
    { label: 'CLONING AND SCANNING REPOSITORY', state: s === 'RUNNING' ? 'active' : s === 'COMPLETED' ? 'done' : s === 'FAILED' ? 'failed' : 'pending' },
    { label: 'DETECTING TECHNOLOGY', state: s === 'COMPLETED' ? 'done' : 'pending' },
    { label: 'FETCHING METADATA', state: s === 'COMPLETED' ? 'done' : 'pending' },
    { label: 'EVALUATING HEALTH', state: s === 'COMPLETED' ? 'done' : 'pending' },
    { label: 'GENERATING INSIGHTS', state: s === 'COMPLETED' ? 'done' : 'pending' },
  ]

  const tone = phase === 'error' ? 'state-error' : phase === 'done' ? 'state-done' : 'state-active'

  return (
    <section className={`analysis-status ${tone}`} aria-live="polite">
      <div className="analysis-status-head">
        <MicroLabel>ANALYSIS JOB</MicroLabel>
        <span className="mono status-badge">{jobStatus || 'UNKNOWN'}</span>
      </div>
      <div className="status-divider" />
      <div className="status-grid mono" role="list">
        {rows.map((row) => (
          <div key={row.label} className={`status-row ${row.state}`} role="listitem">
            <span>{row.label}</span>
            <span className="state-token">{row.state === 'done' ? '✓' : row.state === 'active' ? '…' : row.state === 'failed' ? '✕' : '·'}</span>
          </div>
        ))}
      </div>
      <p className="status-note mono">{statusMsg}</p>
    </section>
  )
}

function ExecutiveSummary({ analysis }) {
  const meta = analysis.metadata || {}
  const insights = analysis.repositoryInsights || {}
  const tech = analysis.technologyAnalysis || {}

  return (
    <section className="panel executive-summary" aria-labelledby="executive-summary-title">
      <MicroLabel>EXECUTIVE SUMMARY</MicroLabel>
      <h2 id="executive-summary-title" className="repo-identity">{meta.fullName || `${meta.owner || 'unknown'} / ${meta.name || 'repository'}`}</h2>
      {meta.description && <p className="repo-description">"{meta.description}"</p>}
      {!meta.description && insights.summary && <p className="repo-description">"{insights.summary}"</p>}

      <div className="summary-readout">
        <div><MicroLabel>TYPE</MicroLabel><p>{tech.repositoryType || 'Unknown'}</p></div>
        <div><MicroLabel>SIZE</MicroLabel><p>{insights.estimatedProjectSize || 'Unknown'}</p></div>
        <div><MicroLabel>MATURITY</MicroLabel><p>{insights.estimatedMaturity || 'Unknown'}</p></div>
        <div><MicroLabel>COMPLEXITY</MicroLabel><p>{insights.estimatedComplexity || 'Unknown'}</p></div>
      </div>
    </section>
  )
}

function MetricsReadout({ analysis }) {
  const languageCount = Object.keys(analysis.languages || {}).length
  const items = [
    ['FILES', analysis.fileCount],
    ['DIRECTORIES', analysis.dirCount],
    ['LINES OF CODE', analysis.totalLines],
    ['LANGUAGES', languageCount],
  ]

  return (
    <section className="panel metrics-readout" aria-label="Core metrics">
      {items.map(([k, v]) => (
        <div key={k} className="metric-cell">
          <MicroLabel>{k}</MicroLabel>
          <p className="mono">{toLocale(v)}</p>
        </div>
      ))}
    </section>
  )
}

function TechnologyStack({ tech }) {
  return (
    <section className="panel">
      <div className="section-title-row">
        <MicroLabel>TECHNOLOGY STACK</MicroLabel>
        {tech?.confidenceScore ? <span className="mono muted">{tech.confidenceScore}% confidence</span> : null}
      </div>
      <div className="status-divider" />
      <div className="tech-grid">
        {TECH_GROUPS.map(([key, label]) => (
          <div key={key} className="tech-row">
            <MicroLabel>{label}</MicroLabel>
            <div className="tech-values">
              {(tech?.[key] || []).length > 0 ? (tech[key].map((item) => (
                <span key={`${label}-${item.name}`} className="tech-tag mono" title={item.reason || ''}>
                  {item.name} {typeof item.confidence === 'number' ? `${item.confidence}%` : ''}
                </span>
              ))) : <span className="mono muted">Not detected</span>}
            </div>
          </div>
        ))}
      </div>
    </section>
  )
}

function LanguageDistribution({ languages }) {
  const grouped = useMemo(() => {
    const all = Object.entries(languages || {}).sort((a, b) => b[1] - a[1])
    const byGroup = { code: [], docs: [], config: [], assets: [] }
    all.forEach(([ext, count]) => byGroup[languageGroup(ext)].push([ext, count]))
    return byGroup
  }, [languages])

  const renderGroup = (key, label) => {
    const rows = grouped[key]
    if (!rows || rows.length === 0) return null
    const total = rows.reduce((sum, [, c]) => sum + c, 0)

    return (
      <div className="language-group" key={key}>
        <MicroLabel>{label}</MicroLabel>
        <div className="language-rows">
          {rows.slice(0, 12).map(([ext, count]) => {
            const pct = total > 0 ? Math.round((count / total) * 100) : 0
            return (
              <div className="language-row" key={ext}>
                <span className="lang-name mono">{ext}</span>
                <div className="lang-bar-track" aria-hidden="true">
                  <div className="lang-bar-fill" style={{ width: `${Math.max(pct, 1)}%`, background: key === 'assets' ? '#565f73' : getLangColor(ext) }} />
                </div>
                <span className="mono lang-value">{count} · {pct}%</span>
              </div>
            )
          })}
        </div>
      </div>
    )
  }

  return (
    <section className="panel">
      <MicroLabel>LANGUAGE DISTRIBUTION</MicroLabel>
      <div className="status-divider" />
      {renderGroup('code', 'Source Code')}
      {renderGroup('docs', 'Documentation')}
      {renderGroup('config', 'Configuration')}
      {renderGroup('assets', 'Assets and Media')}
    </section>
  )
}

function RepositoryHealth({ health }) {
  if (!health) return null

  const categories = [
    ['documentation', 'Documentation'],
    ['testing', 'Testing'],
    ['ciCd', 'CI/CD'],
    ['security', 'Security'],
    ['maintainability', 'Maintainability'],
    ['configuration', 'Configuration'],
    ['projectStructure', 'Project Structure'],
  ]

  return (
    <section className="panel health-panel">
      <MicroLabel>REPOSITORY HEALTH</MicroLabel>
      <div className="health-head">
        <p className="health-score mono">{health.overallScore} / 100</p>
        <p className={`health-grade mono ${gradeClass(health.grade)}`}>GRADE {health.grade}</p>
      </div>

      <div className="health-categories">
        {categories.map(([key, label]) => {
          const c = health.categories?.[key]
          if (!c) return null
          const pct = c.maxScore > 0 ? Math.round((c.score / c.maxScore) * 100) : 0
          return (
            <div className="health-row" key={key}>
              <div className="health-row-top">
                <span>{label}</span>
                <span className={`mono ${healthTone(pct)}`}>{c.score}/{c.maxScore}</span>
              </div>
              <div className="health-track" aria-hidden="true">
                <div className={`health-fill ${healthTone(pct)}`} style={{ width: `${pct}%` }} />
              </div>
            </div>
          )
        })}
      </div>

      <div className="health-notes-grid">
        <div>
          <MicroLabel>STRENGTHS</MicroLabel>
          <ul>
            {(health.strengths || []).slice(0, 5).map((x, i) => <li key={`hs-${i}`}>✓ {x}</li>)}
          </ul>
        </div>
        <div>
          <MicroLabel>WARNINGS</MicroLabel>
          <ul>
            {(health.warnings || []).slice(0, 5).map((x, i) => <li key={`hw-${i}`}>! {x}</li>)}
          </ul>
        </div>
        <div>
          <MicroLabel>RECOMMENDATIONS</MicroLabel>
          <ul>
            {(health.recommendations || []).slice(0, 5).map((x, i) => <li key={`hr-${i}`}>→ {x}</li>)}
          </ul>
        </div>
      </div>
    </section>
  )
}

function EngineeringInsights({ insights }) {
  if (!insights) return null

  return (
    <section className="panel insights-panel">
      <MicroLabel>ENGINEERING INSIGHTS</MicroLabel>
      <div className="status-divider" />

      <div className="insight-block">
        <MicroLabel>SUMMARY</MicroLabel>
        <p>{insights.summary || 'No summary available.'}</p>
      </div>

      <div className="insight-block">
        <MicroLabel>TECHNOLOGY</MicroLabel>
        <p>{insights.technologyOverview || 'No technology overview available.'}</p>
      </div>

      <div className="insight-block">
        <MicroLabel>QUALITY</MicroLabel>
        <p>{insights.qualityOverview || 'No quality overview available.'}</p>
      </div>

      <div className="insight-class-grid">
        <div><MicroLabel>SIZE</MicroLabel><p>{insights.estimatedProjectSize || 'Unknown'}</p></div>
        <div><MicroLabel>MATURITY</MicroLabel><p>{insights.estimatedMaturity || 'Unknown'}</p></div>
        <div><MicroLabel>COMPLEXITY</MicroLabel><p>{insights.estimatedComplexity || 'Unknown'}</p></div>
      </div>

      <div className="insight-notes-grid">
        <div>
          <MicroLabel>STRENGTHS</MicroLabel>
          <ul>
            {(insights.strengths || []).map((x, i) => <li key={`is-${i}`}>✓ {x}</li>)}
          </ul>
        </div>
        <div>
          <MicroLabel>RISKS</MicroLabel>
          <ul>
            {(insights.risks || []).map((x, i) => <li key={`ir-${i}`}>! {x}</li>)}
          </ul>
        </div>
        <div>
          <MicroLabel>RECOMMENDATIONS</MicroLabel>
          <ul>
            {(insights.suggestions || []).map((x, i) => <li key={`ic-${i}`}>→ {x}</li>)}
          </ul>
        </div>
      </div>
    </section>
  )
}

function GithubMetadata({ meta, analyzedAt }) {
  if (!meta) return null
  const rows = [
    ['Stars', meta.stars],
    ['Forks', meta.forks],
    ['Open Issues', meta.openIssues],
    ['Primary Language', meta.primaryLanguage || 'Unknown'],
    ['License', meta.license || 'Unknown'],
    ['Created', meta.createdAt ? new Date(meta.createdAt).toLocaleDateString() : 'Unknown'],
    ['Updated', meta.updatedAt ? new Date(meta.updatedAt).toLocaleDateString() : 'Unknown'],
    ['Topics', (meta.topics || []).length > 0 ? meta.topics.join(', ') : 'None'],
  ]

  return (
    <section className="panel metadata-panel">
      <MicroLabel>GITHUB METADATA</MicroLabel>
      <div className="metadata-grid">
        {rows.map(([k, v]) => (
          <div className="metadata-row" key={k}>
            <span className="mono muted">{k}</span>
            <span>{typeof v === 'number' ? toLocale(v) : v}</span>
          </div>
        ))}
      </div>
      <div className="metadata-links mono">
        <a href={PROJECT_LINKS.repo} target="_blank" rel="noopener noreferrer">Source Repository</a>
        <a href={`https://github.com/${meta.fullName || ''}`} target="_blank" rel="noopener noreferrer">Open Analyzed Repository</a>
        {meta.homepage ? <a href={meta.homepage} target="_blank" rel="noopener noreferrer">Project Homepage</a> : null}
      </div>
      <p className="mono muted">Analyzed at {analyzedAt ? new Date(analyzedAt).toLocaleString() : 'Unknown'}</p>
    </section>
  )
}

function LoadingSkeleton() {
  return (
    <section className="panel loading-panel" aria-label="Loading analysis dashboard">
      <div className="skeleton-line wide" />
      <div className="skeleton-line" />
      <div className="skeleton-line" />
      <div className="skeleton-grid">
        <div className="skeleton-cell" />
        <div className="skeleton-cell" />
        <div className="skeleton-cell" />
        <div className="skeleton-cell" />
      </div>
    </section>
  )
}

function AnalysisDashboard({ analysis }) {
  return (
    <div className="dashboard">
      <ExecutiveSummary analysis={analysis} />
      <MetricsReadout analysis={analysis} />
      <div className="split">
        <TechnologyStack tech={analysis.technologyAnalysis} />
        <LanguageDistribution languages={analysis.languages} />
      </div>
      <RepositoryHealth health={analysis.repositoryHealth} />
      <EngineeringInsights insights={analysis.repositoryInsights} />
      <GithubMetadata meta={analysis.metadata} analyzedAt={analysis.analyzedAt} />
    </div>
  )
}

function Footer() {
  return (
    <footer className="site-footer">
      <div className="footer-inner">
        <p className="footer-title">CORTEX CODE INTELLIGENCE PLATFORM</p>
        <p>Built by Kartick Kumar Ghosh</p>
        <p>C++ Backend Engineer</p>
        <p className="mono">Modern C++ · Distributed Systems · Linux · System Design</p>
        <p className="footer-links">
          <a href={PROFILE_LINKS.github} target="_blank" rel="noopener noreferrer">GitHub</a>
          <span>·</span>
          <a href={PROFILE_LINKS.linkedin} target="_blank" rel="noopener noreferrer">LinkedIn</a>
          <span>·</span>
          <a href={PROFILE_LINKS.resume} target="_blank" rel="noopener noreferrer">Resume</a>
        </p>
        <p className="mono muted">© 2026 Kartick Kumar Ghosh</p>
      </div>
    </footer>
  )
}

export default function App() {
  const [url, setUrl] = useState('')
  const [phase, setPhase] = useState('idle')
  const [statusMsg, setStatusMsg] = useState('')
  const [analysis, setAnalysis] = useState(null)
  const [jobStatus, setJobStatus] = useState('')

  const poll = useCallback(async (jobId) => {
    setPhase('polling')
    const interval = setInterval(async () => {
      try {
        const r = await fetch(`${API}/jobs/${jobId}`)
        const json = await r.json()
        const status = json.data?.status ?? 'UNKNOWN'
        setJobStatus(status)
        setStatusMsg(`Current state: ${status}`)

        if (status === 'COMPLETED') {
          clearInterval(interval)
          const ar = await fetch(`${API}/analysis/${jobId}`)
          const aj = await ar.json()
          if (aj.success) {
            setAnalysis(aj.data)
            setPhase('done')
          } else {
            setStatusMsg(`Analysis unavailable: ${aj.message ?? ''}`)
            setPhase('error')
          }
        } else if (status === 'FAILED') {
          clearInterval(interval)
          setStatusMsg('Analysis failed.')
          setPhase('error')
        }
      } catch (err) {
        clearInterval(interval)
        setStatusMsg(`Error: ${err.message}`)
        setPhase('error')
      }
    }, 2000)
  }, [])

  const handleSubmit = async (e) => {
    e.preventDefault()
    if (!url.trim()) return

    setPhase('submitting')
    setAnalysis(null)
    setJobStatus('QUEUED')
    setStatusMsg('Submitting repository for analysis...')

    try {
      const r = await fetch(`${API}/repositories`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ repositoryUrl: url.trim() }),
      })
      const json = await r.json()

      if (!json.success) {
        setStatusMsg(json.message ?? 'Failed to submit repository.')
        setPhase('error')
        return
      }

      setJobStatus(json.data?.status || 'QUEUED')
      poll(json.data.jobId)
    } catch (err) {
      setStatusMsg(`Request failed: ${err.message}`)
      setPhase('error')
    }
  }

  return (
    <div className="app-shell">
      <div className="texture" aria-hidden="true" />
      <main className="page" role="main">
        <DevHeader />
        <Hero url={url} setUrl={setUrl} phase={phase} onSubmit={handleSubmit} />
        <AnalysisJobStatus phase={phase} statusMsg={statusMsg} jobStatus={jobStatus} />
        {phase === 'polling' ? <LoadingSkeleton /> : null}
        {phase === 'done' && analysis ? <AnalysisDashboard analysis={analysis} /> : null}
      </main>
      <Footer />
    </div>
  )
}
