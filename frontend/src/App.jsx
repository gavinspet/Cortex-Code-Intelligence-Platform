import { useState, useCallback } from 'react'

// Determine API URL based on environment
const getApiUrl = () => {
  const envUrl = import.meta.env.VITE_API_URL
  if (envUrl) return envUrl
  // Production: use Render backend
  if (typeof window !== 'undefined' && window.location.hostname.includes('vercel.app')) {
    return 'https://cortex-code-intelligence-platform.onrender.com'
  }
  // Development: use localhost
  return 'http://localhost:8080'
}

const API = getApiUrl()

function LanguageBar({ name, count, total }) {
  const pct = total > 0 ? Math.round((count / total) * 100) : 0
  return (
    <div className="lang-row">
      <span className="lang-name">{name}</span>
      <div className="lang-bar-wrap">
        <div className="lang-bar" style={{ width: `${pct}%` }} />
      </div>
      <span className="lang-count">{count} files ({pct}%)</span>
    </div>
  )
}

export default function App() {
  const [url, setUrl] = useState('')
  const [phase, setPhase] = useState('idle')   // idle | submitting | polling | done | error
  const [statusMsg, setStatusMsg] = useState('')
  const [jobStatus, setJobStatus] = useState('')
  const [analysis, setAnalysis] = useState(null)

  const poll = useCallback(async (jobId) => {
    setPhase('polling')
    const interval = setInterval(async () => {
      try {
        const r = await fetch(`${API}/jobs/${jobId}`)
        const json = await r.json()
        const status = json.data?.status ?? 'UNKNOWN'
        setJobStatus(status)
        setStatusMsg(`Job ${jobId.slice(0, 8)}… — ${status}`)

        if (status === 'COMPLETED') {
          clearInterval(interval)
          // Fetch analysis results
          const ar = await fetch(`${API}/analysis/${jobId}`)
          const aj = await ar.json()
          if (aj.success) {
            setAnalysis(aj.data)
            setPhase('done')
          } else {
            setStatusMsg('Analysis data not available: ' + (aj.message ?? ''))
            setPhase('error')
          }
        } else if (status === 'FAILED') {
          clearInterval(interval)
          setStatusMsg('Job failed. Check server logs.')
          setPhase('error')
        }
      } catch (err) {
        clearInterval(interval)
        setStatusMsg('Polling error: ' + err.message)
        setPhase('error')
      }
    }, 2000)
  }, [])

  const handleSubmit = async (e) => {
    e.preventDefault()
    if (!url.trim()) return

    setPhase('submitting')
    setAnalysis(null)
    setStatusMsg('Submitting repository…')
    setJobStatus('')

    try {
      const r = await fetch(`${API}/repositories`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ repositoryUrl: url.trim() })
      })
      const json = await r.json()
      if (!json.success) {
        setStatusMsg(json.message ?? 'Submission failed')
        setPhase('error')
        return
      }
      const jobId = json.data.jobId
      setStatusMsg(`Job created: ${jobId.slice(0, 8)}…`)
      poll(jobId)
    } catch (err) {
      setStatusMsg('Request failed: ' + err.message)
      setPhase('error')
    }
  }

  const sortedLanguages = analysis
    ? Object.entries(analysis.languages).sort((a, b) => b[1] - a[1])
    : []

  const totalLangFiles = sortedLanguages.reduce((s, [, c]) => s + c, 0)

  return (
    <div className="app-wrapper">
      <div className="container">
        {/* Header with badges */}
        <header className="header-premium">
          <div className="header-top">
            <div className="header-title-group">
              <h1>Cortex <span>Code Intelligence</span></h1>
              <div className="badges">
                <span className="badge badge-version">v1.0.0</span>
                <span className="badge badge-status">● Live</span>
              </div>
            </div>
            <a href="https://github.com/gavinspet/Cortex-Code-Intelligence-Platform" target="_blank" rel="noopener noreferrer" className="btn-source">
              <svg width="16" height="16" viewBox="0 0 16 16" fill="currentColor">
                <path d="M8 0C3.58 0 0 3.58 0 8c0 3.54 2.29 6.53 5.47 7.59.4.07.55-.17.55-.38 0-.19-.01-.82-.01-1.49-2.01.37-2.53-.49-2.69-.94-.09-.23-.48-.94-.82-1.13-.28-.15-.68-.52-.01-.53.63-.01 1.08.58 1.23.82.72 1.21 1.87.87 2.33.66.07-.52.28-.87.51-1.07-1.78-.2-3.64-.89-3.64-3.95 0-.87.31-1.59.82-2.15-.08-.2-.36-1.02.08-2.12 0 0 .67-.21 2.2.82.64-.18 1.32-.27 2-.27.68 0 1.36.09 2 .27 1.53-1.04 2.2-.82 2.2-.82.44 1.1.16 1.92.08 2.12.51.56.82 1.27.82 2.15 0 3.07-1.87 3.75-3.65 3.95.29.25.54.73.54 1.48 0 1.07-.01 1.93-.01 2.2 0 .21.15.46.55.38A8.012 8.012 0 0 0 16 8c0-4.42-3.58-8-8-8z" />
              </svg>
              View Source
            </a>
          </div>
          <p className="header-subtitle">Analyze any public GitHub repository</p>
        </header>

        {/* Hero Section */}
        <section className="hero">
          <form className="form-premium" onSubmit={handleSubmit}>
            <input
              type="text"
              className="url-input-premium"
              placeholder="https://github.com/user/repo.git"
              value={url}
              onChange={e => setUrl(e.target.value)}
              disabled={phase === 'submitting' || phase === 'polling'}
            />
            <button
              type="submit"
              className="btn-primary"
              disabled={phase === 'submitting' || phase === 'polling' || !url.trim()}
            >
              {phase === 'submitting' || phase === 'polling' ? 'Analyzing…' : 'Analyze'}
            </button>
          </form>
          <p className="hero-caption">Analyze public GitHub repositories using a high-performance C++20 backend.</p>
        </section>

        {/* Status Messages */}
        {statusMsg && (
          <div className={`status-premium ${phase === 'error' ? 'status-error' : ''} ${phase === 'done' ? 'status-done' : ''}`}>
            {phase === 'polling' && <span className="spinner" />}
            {statusMsg}
          </div>
        )}

        {phase === 'polling' && jobStatus && (
          <div className="job-status-premium">
            Status: <strong>{jobStatus}</strong>
            {jobStatus === 'RUNNING' && ' — cloning and scanning repository…'}
            {jobStatus === 'QUEUED' && ' — waiting in queue…'}
          </div>
        )}

        {/* Results Card */}
        {analysis && (
          <section className="results-premium">
            <div className="results-header">
              <h2>Analysis Results</h2>
              <p className="results-timestamp">Analyzed at {new Date(analysis.analyzedAt).toLocaleString()}</p>
            </div>
            
            <div className="metrics-premium">
              <div className="metric-premium">
                <div className="metric-value-premium">{analysis.fileCount.toLocaleString()}</div>
                <div className="metric-label-premium">Files</div>
              </div>
              <div className="metric-premium">
                <div className="metric-value-premium">{analysis.dirCount.toLocaleString()}</div>
                <div className="metric-label-premium">Directories</div>
              </div>
              <div className="metric-premium">
                <div className="metric-value-premium">{analysis.totalLines.toLocaleString()}</div>
                <div className="metric-label-premium">Lines of Code</div>
              </div>
              <div className="metric-premium">
                <div className="metric-value-premium">{sortedLanguages.length}</div>
                <div className="metric-label-premium">Languages</div>
              </div>
            </div>

            {sortedLanguages.length > 0 && (
              <div className="languages-premium">
                <h3>Language Distribution</h3>
                <div className="languages-list">
                  {sortedLanguages.slice(0, 15).map(([ext, count]) => (
                    <LanguageBar key={ext} name={ext} count={count} total={totalLangFiles} />
                  ))}
                </div>
              </div>
            )}
          </section>
        )}
      </div>

      {/* Professional Footer */}
      <footer className="footer-premium">
        <div className="footer-content">
          <div className="footer-section footer-left">
            <div className="footer-brand">
              <h3>Cortex Code Intelligence Platform</h3>
              <p>Production-grade repository intelligence platform.</p>
            </div>
            <div className="footer-stack">
              <span className="stack-label">Built using</span>
              <div className="stack-items">
                <span>C++20</span>
                <span>Drogon</span>
                <span>React</span>
                <span>Docker</span>
                <span>GitHub Actions</span>
              </div>
            </div>
          </div>

          <div className="footer-section footer-center">
            <h4>Quick Links</h4>
            <ul>
              <li><a href="https://github.com/gavinspet/Cortex-Code-Intelligence-Platform" target="_blank" rel="noopener noreferrer">GitHub Repository</a></li>
              <li><a href="https://cortex-code-intelligence-platform.onrender.com" target="_blank" rel="noopener noreferrer">Backend Status</a></li>
              <li><a href="https://cortex-code-intelligence-platform.onrender.com/health" target="_blank" rel="noopener noreferrer">Health Check</a></li>
              <li><a href="https://github.com/gavinspet/Cortex-Code-Intelligence-Platform/blob/main/docs/API.md" target="_blank" rel="noopener noreferrer">API Documentation</a></li>
            </ul>
          </div>

          <div className="footer-section footer-right">
            <h4>About the Developer</h4>
            <div className="developer-info">
              <div>
                <div className="dev-name">Kartick Kumar Ghosh</div>
                <div className="dev-role">Software Engineer</div>
              </div>
              <div className="specialization">
                <span class="spec-label">Specializing in</span>
                <div class="spec-items">
                  <span>Modern C++</span>
                  <span>Backend Engineering</span>
                  <span>Distributed Systems</span>
                  <span>System Design</span>
                </div>
              </div>
              <div className="social-links">
                <a href="https://github.com/gavinspet" target="_blank" rel="noopener noreferrer" className="social-btn" title="GitHub">
                  <svg width="20" height="20" viewBox="0 0 16 16" fill="currentColor">
                    <path d="M8 0C3.58 0 0 3.58 0 8c0 3.54 2.29 6.53 5.47 7.59.4.07.55-.17.55-.38 0-.19-.01-.82-.01-1.49-2.01.37-2.53-.49-2.69-.94-.09-.23-.48-.94-.82-1.13-.28-.15-.68-.52-.01-.53.63-.01 1.08.58 1.23.82.72 1.21 1.87.87 2.33.66.07-.52.28-.87.51-1.07-1.78-.2-3.64-.89-3.64-3.95 0-.87.31-1.59.82-2.15-.08-.2-.36-1.02.08-2.12 0 0 .67-.21 2.2.82.64-.18 1.32-.27 2-.27.68 0 1.36.09 2 .27 1.53-1.04 2.2-.82 2.2-.82.44 1.1.16 1.92.08 2.12.51.56.82 1.27.82 2.15 0 3.07-1.87 3.75-3.65 3.95.29.25.54.73.54 1.48 0 1.07-.01 1.93-.01 2.2 0 .21.15.46.55.38A8.012 8.012 0 0 0 16 8c0-4.42-3.58-8-8-8z" />
                  </svg>
                </a>
                <a href="mailto:kartick.ghosh.dev@gmail.com" className="social-btn" title="Email">
                  <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                    <path d="M4 4h16c1.1 0 2 .9 2 2v12c0 1.1-.9 2-2 2H4c-1.1 0-2-.9-2-2V6c0-1.1.9-2 2-2z"></path>
                    <polyline points="22,6 12,13 2,6"></polyline>
                  </svg>
                </a>
              </div>
            </div>
          </div>
        </div>

        <div className="footer-divider"></div>

        <div className="footer-bottom">
          <p>Copyright © 2026 Kartick Kumar Ghosh. Licensed under the MIT License.</p>
          <p>Made with C++20, React and ☕</p>
          <p className="version-footer">Version 1.0.0</p>
        </div>
      </footer>
    </div>
  )
}
