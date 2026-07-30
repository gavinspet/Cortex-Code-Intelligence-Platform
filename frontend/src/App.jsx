import { useState, useCallback } from 'react'

const API = ''  // proxied by Vite dev server

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
    <div className="container">
      <header>
        <h1>Cortex <span>Code Intelligence</span></h1>
        <p>Analyze any public GitHub repository</p>
      </header>

      <form className="form" onSubmit={handleSubmit}>
        <input
          type="text"
          className="url-input"
          placeholder="https://github.com/user/repo.git"
          value={url}
          onChange={e => setUrl(e.target.value)}
          disabled={phase === 'submitting' || phase === 'polling'}
        />
        <button
          type="submit"
          className="btn"
          disabled={phase === 'submitting' || phase === 'polling' || !url.trim()}
        >
          {phase === 'submitting' || phase === 'polling' ? 'Analyzing…' : 'Analyze'}
        </button>
      </form>

      {statusMsg && (
        <div className={`status ${phase === 'error' ? 'status-error' : ''} ${phase === 'done' ? 'status-done' : ''}`}>
          {phase === 'polling' && <span className="spinner" />}
          {statusMsg}
        </div>
      )}

      {phase === 'polling' && jobStatus && (
        <div className="job-status">
          Status: <strong>{jobStatus}</strong>
          {jobStatus === 'RUNNING' && ' — cloning and scanning repository…'}
          {jobStatus === 'QUEUED' && ' — waiting in queue…'}
        </div>
      )}

      {analysis && (
        <section className="results">
          <h2>Analysis Results</h2>
          <div className="metrics">
            <div className="metric">
              <div className="metric-value">{analysis.fileCount.toLocaleString()}</div>
              <div className="metric-label">Files</div>
            </div>
            <div className="metric">
              <div className="metric-value">{analysis.dirCount.toLocaleString()}</div>
              <div className="metric-label">Directories</div>
            </div>
            <div className="metric">
              <div className="metric-value">{analysis.totalLines.toLocaleString()}</div>
              <div className="metric-label">Lines of Code</div>
            </div>
            <div className="metric">
              <div className="metric-value">{sortedLanguages.length}</div>
              <div className="metric-label">Languages</div>
            </div>
          </div>

          {sortedLanguages.length > 0 && (
            <div className="languages">
              <h3>Language Distribution</h3>
              {sortedLanguages.slice(0, 15).map(([ext, count]) => (
                <LanguageBar key={ext} name={ext} count={count} total={totalLangFiles} />
              ))}
            </div>
          )}

          <div className="analyzed-at">
            Analyzed at {new Date(analysis.analyzedAt).toLocaleString()}
          </div>
        </section>
      )}
    </div>
  )
}
