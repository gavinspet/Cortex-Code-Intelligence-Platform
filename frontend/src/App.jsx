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
  linkedin: 'https://www.linkedin.com/in/kartick-kumar-ghosh-779679190/',
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

function parseRepoFromUrl(rawUrl) {
  if (!rawUrl) return null
  try {
    const u = new URL(rawUrl)
    const parts = u.pathname.replace(/\.git$/, '').split('/').filter(Boolean)
    if (parts.length >= 2) {
      return {
        fullName: `${parts[0]} / ${parts[1]}`,
        githubLink: `${u.protocol}//${u.host}/${parts[0]}/${parts[1]}`,
      }
    }
  } catch {
    return null
  }
  return null
}

function resolveRepositoryIdentity(meta, submittedUrl) {
  const fromUrl = parseRepoFromUrl(submittedUrl)

  if (meta?.fullName && meta.fullName.trim()) {
    return { label: meta.fullName, githubLink: `https://github.com/${meta.fullName}` }
  }

  if (meta?.owner && meta?.name) {
    return { label: `${meta.owner} / ${meta.name}`, githubLink: `https://github.com/${meta.owner}/${meta.name}` }
  }

  if (meta?.name) {
    return { label: meta.name, githubLink: fromUrl?.githubLink || null }
  }

  if (fromUrl?.fullName) {
    return { label: fromUrl.fullName, githubLink: fromUrl.githubLink }
  }

  return { label: 'Repository Analysis', githubLink: null }
}

function MicroLabel({ children }) {
  return <p className="micro-label">{children}</p>
}

function DevHeader() {
  return (
    <header className="developer-bar">
      <div className="developer-intro">
        <MicroLabel>Built by</MicroLabel>
        <p className="dev-name">KARTICK KUMAR GHOSH</p>
        <p className="developer-role">Software Developer · C++ Backend and Systems Engineer</p>
        <p className="developer-summary">Designing reliable backend services and developer tooling with a strong focus on performance, API quality, and maintainable architecture.</p>
        <p className="developer-focus">C++20 · Linux · Distributed Systems · REST APIs · System Design</p>
        <div className="developer-stats" aria-label="Developer highlights">
          <span className="developer-stat">Backend-first engineering</span>
          <span className="developer-stat">Production-minded architecture</span>
          <span className="developer-stat">Open source workflow</span>
        </div>
      </div>
      <nav className="developer-links" aria-label="Developer links">
        <a href={PROFILE_LINKS.github} target="_blank" rel="noopener noreferrer"><span aria-hidden="true">◉</span>GitHub</a>
        <a href={PROFILE_LINKS.linkedin} target="_blank" rel="noopener noreferrer"><span aria-hidden="true">◍</span>LinkedIn</a>
      </nav>
    </header>
  )
}

function Hero({ url, setUrl, phase, onSubmit }) {
  return (
    <section className="hero" aria-labelledby="cortex-title">
      <p className="hero-kicker">Cortex</p>
      <h1 id="cortex-title">Code Intelligence Platform</h1>
      <p className="hero-subtitle">Analyze any public GitHub repository.</p>

      <form className="search-form" onSubmit={onSubmit}>
        <label className="sr-only" htmlFor="repositoryUrl">Repository URL</label>
        <input
          id="repositoryUrl"
          type="text"
          value={url}
          onChange={(e) => setUrl(e.target.value)}
          placeholder="https://github.com/user/repository"
          className="search-input mono"
          disabled={phase === 'submitting' || phase === 'polling'}
        />
        <button
          type="submit"
          className="search-button"
          disabled={phase === 'submitting' || phase === 'polling' || !url.trim()}
        >
          {phase === 'submitting' || phase === 'polling' ? 'Analyzing...' : 'Analyze'}
        </button>
      </form>

      <p className="hero-meta">Static analysis · Repository intelligence · Engineering insights</p>
    </section>
  )
}

function AnalysisJobStatus({ phase, statusMsg, jobStatus }) {
  if (!statusMsg) return null

  const s = (jobStatus || '').toUpperCase()
  const steps = [
    { label: 'Repository submitted', done: s === 'QUEUED' || s === 'RUNNING' || s === 'COMPLETED' },
    { label: 'Repository cloned', done: s === 'RUNNING' || s === 'COMPLETED' },
    { label: 'Files scanned', done: s === 'RUNNING' || s === 'COMPLETED' },
    { label: 'Technology detected', done: s === 'COMPLETED' },
    { label: 'GitHub metadata fetched', done: s === 'COMPLETED' },
    { label: 'Evaluating repository health', done: s === 'COMPLETED', active: s === 'RUNNING' },
  ]

  return (
    <section className={`live-status ${phase === 'error' ? 'error' : ''}`} aria-live="polite">
      <p className="live-title">{phase === 'done' ? 'Analysis complete' : 'Analyzing repository...'}</p>
      {phase !== 'error' && (
        <ul>
          {steps.map((step) => (
            <li key={step.label}>
              <span className={step.done ? 'done' : step.active ? 'active' : 'pending'}>{step.done ? '✓' : step.active ? '●' : '○'}</span>
              {step.label}
            </li>
          ))}
        </ul>
      )}
      <p className="status-message">{statusMsg}</p>
    </section>
  )
}

function ExecutiveSummary({ analysis, submittedUrl }) {
  const meta = analysis.metadata || null
  const insights = analysis.repositoryInsights || null
  const tech = analysis.technologyAnalysis || null
  const identity = resolveRepositoryIdentity(meta, submittedUrl)

  return (
    <section id="overview" className="section" aria-labelledby="executive-summary-title">
      <MicroLabel>Repository Analysis</MicroLabel>
      <h2 id="executive-summary-title">{identity.label}</h2>
      {meta?.description && <p className="summary-text">{meta.description}</p>}
      {!meta?.description && insights?.summary && <p className="summary-text">{insights.summary}</p>}
      {identity.githubLink && <p className="summary-text mono">{identity.githubLink}</p>}

      <div className="classification-grid">
        <div><MicroLabel>Project Type</MicroLabel><p>{tech?.repositoryType || 'Unknown'}</p></div>
        <div><MicroLabel>Size</MicroLabel><p>{insights?.estimatedProjectSize || 'Unknown'}</p></div>
        <div><MicroLabel>Maturity</MicroLabel><p>{insights?.estimatedMaturity || 'Unknown'}</p></div>
        <div><MicroLabel>Complexity</MicroLabel><p>{insights?.estimatedComplexity || 'Unknown'}</p></div>
      </div>
    </section>
  )
}

function MetricsReadout({ analysis }) {
  const languageCount = Object.keys(analysis.languages || {}).length
  const items = [
    { label: 'Files', value: analysis.fileCount, icon: '▦', tone: 'tone-blue' },
    { label: 'Dirs', value: analysis.dirCount, icon: '◫', tone: 'tone-violet' },
    { label: 'Lines', value: analysis.totalLines, icon: '≣', tone: 'tone-teal' },
    { label: 'Languages', value: languageCount, icon: '◎', tone: 'tone-amber' },
  ]

  return (
    <section className="metrics-row" aria-label="Core metrics">
      {items.map((item) => (
        <div key={item.label} className={`metric-item ${item.tone}`}>
          <p className="metric-icon" aria-hidden="true">{item.icon}</p>
          <p className="metric-value">{toLocale(item.value)}</p>
          <p className="metric-label">{item.label}</p>
        </div>
      ))}
    </section>
  )
}

function TechnologyStack({ tech }) {
  if (!tech) return null

  return (
    <section id="technology" className="section">
      <div className="section-head">
        <h3><span className="section-icon icon-tech" aria-hidden="true">◈</span>Technology Stack</h3>
        {typeof tech.confidenceScore === 'number'
          ? <p className="head-sub">Detected automatically · {tech.confidenceScore}% confidence</p>
          : <p className="head-sub">Detected automatically</p>}
      </div>
      <div className="tech-grid">
        {TECH_GROUPS.map(([key, label]) => (
          <div key={key} className="tech-group">
            <MicroLabel>{label}</MicroLabel>
            <div className="chip-wrap">
              {(tech?.[key] || []).length > 0 ? (tech[key].map((item) => (
                <span key={`${label}-${item.name}`} className="chip" title={item.reason || ''}>
                  {item.name}{typeof item.confidence === 'number' ? ` ${item.confidence}%` : ''}
                </span>
              ))) : null}
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
        <div className="language-table">
          {rows.slice(0, 12).map(([ext, count]) => {
            const pct = total > 0 ? Math.round((count / total) * 100) : 0
            return (
              <div className="language-row" key={ext}>
                <span className="language-name">{ext}</span>
                <span className="language-files">{count}</span>
                <div className="bar-track" aria-hidden="true">
                  <div className="bar-fill" style={{ width: `${Math.max(pct, 1)}%`, background: key === 'assets' ? '#b0b7c3' : getLangColor(ext) }} />
                </div>
                <span className="language-pct">{pct}%</span>
              </div>
            )
          })}
        </div>
      </div>
    )
  }

  return (
    <section id="languages" className="section">
      <h3><span className="section-icon icon-language" aria-hidden="true">◍</span>Language Analysis</h3>
      {renderGroup('code', 'Source Code')}
      {renderGroup('docs', 'Documentation')}
      {renderGroup('config', 'Configuration')}
      {renderGroup('assets', 'Assets and Media')}
    </section>
  )
}

function RepositoryHealth({ health }) {
  if (!health) return null

  const gradeTone = gradeClass(health.grade).replace('is-', '')

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
    <section id="health" className="section">
      <h3><span className="section-icon icon-health" aria-hidden="true">◔</span>Repository Health</h3>
      <div className="health-overview">
        <p className="health-score">{health.overallScore} / 100</p>
        <p className={`health-grade ${gradeTone}`}>Grade {health.grade}</p>
      </div>

      <div className="health-list">
        {categories.map(([key, label]) => {
          const c = health.categories?.[key]
          if (!c) return null
          const pct = c.maxScore > 0 ? Math.round((c.score / c.maxScore) * 100) : 0
          const tone = healthTone(pct).replace('is-', '')
          return (
            <div className="health-row" key={key}>
              <div className="health-row-header">
                <span>{label}</span>
                <span>{c.score}/{c.maxScore} {c.grade}</span>
              </div>
              <div className="health-bar" aria-hidden="true">
                <div className={`health-fill ${tone}`} style={{ width: `${pct}%` }} />
              </div>
            </div>
          )
        })}
      </div>
    </section>
  )
}

function EngineeringInsights({ insights }) {
  if (!insights) return null

  return (
    <section id="insights" className="section">
      <h3><span className="section-icon icon-insights" aria-hidden="true">◇</span>Engineering Insights</h3>

      <div className="insight-text-block">
        <MicroLabel>Summary</MicroLabel>
        <p>{insights.summary || 'No summary available.'}</p>
      </div>

      <div className="insight-text-block">
        <MicroLabel>Technology</MicroLabel>
        <p>{insights.technologyOverview || 'No technology overview available.'}</p>
      </div>

      <div className="insight-text-block">
        <MicroLabel>Quality</MicroLabel>
        <p>{insights.qualityOverview || 'No quality overview available.'}</p>
      </div>

      <div className="insight-grid">
        <div>
          <MicroLabel>Strengths</MicroLabel>
          <ul>
            {(insights.strengths || []).map((x, i) => <li key={`is-${i}`}>✓ {x}</li>)}
          </ul>
        </div>
        <div>
          <MicroLabel>Risks</MicroLabel>
          <ul>
            {(insights.risks || []).map((x, i) => <li key={`ir-${i}`}>! {x}</li>)}
          </ul>
        </div>
        <div>
          <MicroLabel>Recommendations</MicroLabel>
          <ul>
            {(insights.suggestions || []).map((x, i) => <li key={`ic-${i}`}>→ {x}</li>)}
          </ul>
        </div>
      </div>
    </section>
  )
}

function GithubMetadata({ meta }) {
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
    <section id="github" className="section">
      <h3><span className="section-icon icon-github" aria-hidden="true">◉</span>GitHub Repository</h3>
      <div className="metadata-grid">
        {rows.map(([k, v]) => (
          <div className="metadata-row" key={k}>
            <span>{k}</span>
            <span className="metadata-value">{typeof v === 'number' ? toLocale(v) : v}</span>
          </div>
        ))}
      </div>
    </section>
  )
}

function SectionNav({ visible }) {
  if (!visible) return null

  return (
    <nav className="section-nav" aria-label="Results section navigation">
      <a href="#overview">Overview</a>
      <a href="#technology">Technology</a>
      <a href="#languages">Languages</a>
      <a href="#health">Health</a>
      <a href="#insights">Insights</a>
      <a href="#github">GitHub</a>
    </nav>
  )
}

function AnalysisDashboard({ analysis, submittedUrl }) {
  return (
    <div className="results">
      <ExecutiveSummary analysis={analysis} submittedUrl={submittedUrl} />
      <MetricsReadout analysis={analysis} />
      {analysis.technologyAnalysis ? <TechnologyStack tech={analysis.technologyAnalysis} /> : null}
      <LanguageDistribution languages={analysis.languages} />
      {analysis.repositoryHealth ? <RepositoryHealth health={analysis.repositoryHealth} /> : null}
      {analysis.repositoryInsights ? <EngineeringInsights insights={analysis.repositoryInsights} /> : null}
      {analysis.metadata ? <GithubMetadata meta={analysis.metadata} /> : null}
    </div>
  )
}

function Footer() {
  return (
    <footer className="footer">
      <div className="footer-inner">
        <div className="footer-block">
          <p className="footer-title">Cortex Code Intelligence Platform</p>
          <p className="footer-desc">Analyze public repositories with automated technology detection, health scoring, and actionable engineering insights.</p>
          <p className="footer-copy">© 2026 Kartick Kumar Ghosh</p>
        </div>

        <div className="footer-block">
          <p className="footer-label">Platform</p>
          <p>Static code analysis</p>
          <p>Repository health intelligence</p>
          <p>Architecture and quality signals</p>
        </div>

        <div className="footer-block">
          <p className="footer-label">Links</p>
          <p className="footer-links">
            <a href={PROJECT_LINKS.repo} target="_blank" rel="noopener noreferrer">Project Repo</a>
          </p>
          <p className="footer-links">
            <a href={PROJECT_LINKS.health} target="_blank" rel="noopener noreferrer">API Health</a>
          </p>
          <p className="footer-links">
            <a href={PROFILE_LINKS.github} target="_blank" rel="noopener noreferrer">GitHub</a>
            <span>·</span>
            <a href={PROFILE_LINKS.linkedin} target="_blank" rel="noopener noreferrer">LinkedIn</a>
          </p>
        </div>
      </div>
    </footer>
  )
}

export default function App() {
  const [url, setUrl] = useState('')
  const [submittedUrl, setSubmittedUrl] = useState('')
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
            setStatusMsg('Analysis complete')
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
    const cleanUrl = url.trim()
    if (!cleanUrl) return

    setSubmittedUrl(cleanUrl)
    setPhase('submitting')
    setAnalysis(null)
    setJobStatus('QUEUED')
    setStatusMsg('Submitting repository for analysis...')

    try {
      const r = await fetch(`${API}/repositories`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ repositoryUrl: cleanUrl }),
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
      <main className="page" role="main">
        <Hero url={url} setUrl={setUrl} phase={phase} onSubmit={handleSubmit} />
        <SectionNav visible={phase === 'done' && !!analysis} />
        <AnalysisJobStatus phase={phase} statusMsg={statusMsg} jobStatus={jobStatus} />
        {phase === 'done' && analysis ? <AnalysisDashboard analysis={analysis} submittedUrl={submittedUrl} /> : null}
      </main>
      <Footer />
    </div>
  )
}
