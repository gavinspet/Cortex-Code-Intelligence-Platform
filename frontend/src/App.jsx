import { useState, useCallback } from 'react'

// ─── API URL ─────────────────────────────────────────────────────────────────

const getApiUrl = () => {
  const e = import.meta.env.VITE_API_URL
  if (e) return e
  if (typeof window !== 'undefined' && window.location.hostname.includes('vercel.app'))
    return 'https://cortex-code-intelligence-platform.onrender.com'
  return 'http://localhost:8080'
}
const API = getApiUrl()

// ─── Language colour map ──────────────────────────────────────────────────────

const LANG_COLORS = {
  '.js':'#f7df1e','.jsx':'#61dafb','.ts':'#3178c6','.tsx':'#61dafb',
  '.py':'#3572a5','.cpp':'#f34b7d','.cc':'#f34b7d','.cxx':'#f34b7d',
  '.c':'#555555','.h':'#6e4c13','.hpp':'#f34b7d','.java':'#b07219',
  '.go':'#00add8','.rs':'#dea584','.rb':'#701516','.php':'#4f5d95',
  '.cs':'#178600','.swift':'#f05138','.kt':'#7f52ff','.vue':'#41b883',
  '.svelte':'#ff3e00','.html':'#e34c26','.css':'#563d7c','.scss':'#c6538c',
  '.md':'#083fa1','.json':'#cbcb41','.yml':'#cb171e','.yaml':'#cb171e',
  '.sh':'#89e051','.sql':'#e38c00','.xml':'#0060ac','.toml':'#9c4221',
}
const ASSET_EXTS  = new Set(['.png','.jpg','.jpeg','.gif','.svg','.ico','.mp3','.mp4','.wav','.webp','.woff','.woff2','.ttf','.otf','.eot'])
const DOC_EXTS    = new Set(['.md','.rst','.txt','.adoc'])
const CONFIG_EXTS = new Set(['.json','.yaml','.yml','.toml','.xml','.ini','.env','.cfg','.conf'])
const getLangColor = ext => LANG_COLORS[ext] || '#6366f1'

function extGroup(ext) {
  if (ASSET_EXTS.has(ext))  return 'assets'
  if (DOC_EXTS.has(ext))    return 'docs'
  if (CONFIG_EXTS.has(ext)) return 'config'
  return 'code'
}

// ─── Colour helpers ───────────────────────────────────────────────────────────

function gradeColor(g) {
  return { A:'#34d399', B:'#86efac', C:'#fbbf24', D:'#f97316', F:'#f87171' }[g] || '#94a3b8'
}
function healthColor(pct) {
  if (pct >= 80) return '#34d399'
  if (pct >= 60) return '#fbbf24'
  if (pct >= 40) return '#f97316'
  return '#f87171'
}
function complexityColor(c) {
  return { Low:'#34d399', Medium:'#fbbf24', High:'#f97316', 'Very High':'#f87171' }[c] || '#94a3b8'
}
function maturityColor(m) {
  return { Prototype:'#f87171', 'Personal Project':'#f97316', 'Production Ready':'#fbbf24',
    'Open Source Library':'#86efac', 'Enterprise Grade':'#34d399' }[m] || '#94a3b8'
}
function sizeColor(s) {
  return { Tiny:'#94a3b8', Small:'#86efac', Medium:'#fbbf24',
    Large:'#f97316', Enterprise:'#a78bfa' }[s] || '#94a3b8'
}

// ─── Primitive atoms ──────────────────────────────────────────────────────────

function SectionHeader({ icon, title, badge, badgeVariant = 'default' }) {
  return (
    <div className="card-section-header">
      <span className="section-icon">{icon}</span>
      <h3>{title}</h3>
      {badge && <span className={`section-badge badge-${badgeVariant}`}>{badge}</span>}
    </div>
  )
}

function StatPill({ icon, value, label }) {
  return (
    <div className="stat-pill">
      <span className="stat-pill-icon">{icon}</span>
      <span className="stat-pill-value">{typeof value === 'number' ? value.toLocaleString() : value}</span>
      <span className="stat-pill-label">{label}</span>
    </div>
  )
}

function TechBadge({ name, confidence }) {
  return (
    <span className="tech-badge" title={confidence ? `${confidence}% confidence` : undefined}>
      {name}
    </span>
  )
}

function NotDetected() {
  return <span className="not-detected">Not detected</span>
}

function InsightItem({ text, type }) {
  const icon = type === 'strength'
    ? <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#34d399" strokeWidth="2.5"><polyline points="20 6 9 17 4 12"/></svg>
    : <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="#fbbf24" strokeWidth="2"><path d="M10.29 3.86L1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z"/><line x1="12" y1="9" x2="12" y2="13"/><line x1="12" y1="17" x2="12.01" y2="17"/></svg>
  return (
    <div className={`insight-item ${type}`}>
      <span className="insight-icon">{icon}</span>
      <span className="insight-text">{text}</span>
    </div>
  )
}

// ─── Section 2: Executive Summary ─────────────────────────────────────────────

function ExecutiveSummaryCard({ insights }) {
  if (!insights?.summary) return null
  return (
    <div className="dash-card executive-summary animate-in">
      <div className="exec-header">
        <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
          <circle cx="12" cy="12" r="10"/><line x1="12" y1="8" x2="12" y2="12"/><line x1="12" y1="16" x2="12.01" y2="16"/>
        </svg>
        <span>Executive Summary</span>
      </div>
      <p className="exec-summary-text">{insights.summary}</p>
      {insights.technologyOverview && (
        <p className="exec-tech-text">{insights.technologyOverview}</p>
      )}
    </div>
  )
}

// ─── Section 1: Repository Overview ──────────────────────────────────────────

function RepositoryOverviewCard({ meta, analyzedAt }) {
  if (!meta) return null
  return (
    <div className="dash-card repo-overview animate-in">
      <div className="repo-top">
        <div className="repo-identity">
          {meta.ownerAvatarUrl
            ? <img src={meta.ownerAvatarUrl} alt={meta.owner} className="owner-avatar" />
            : <div className="owner-avatar-placeholder">{(meta.owner||'?')[0].toUpperCase()}</div>
          }
          <div className="repo-title-group">
            <div className="repo-name-row">
              <h2 className="repo-name">{meta.name}</h2>
              <span className={`visibility-badge vis-${meta.visibility}`}>{meta.visibility}</span>
              {meta.archived && <span className="status-chip chip-archived">archived</span>}
              {meta.fork && <span className="status-chip chip-fork">fork</span>}
            </div>
            <div className="repo-byline">
              <span className="repo-owner-link">{meta.owner}</span>
              <span className="byline-sep">/</span>
              <span className="repo-full-name">{meta.name}</span>
            </div>
          </div>
        </div>
        <div className="repo-links">
          <a href={`https://github.com/${meta.fullName}`} target="_blank" rel="noopener noreferrer" className="icon-btn" title="Open on GitHub">
            <svg width="18" height="18" viewBox="0 0 16 16" fill="currentColor">
              <path d="M8 0C3.58 0 0 3.58 0 8c0 3.54 2.29 6.53 5.47 7.59.4.07.55-.17.55-.38 0-.19-.01-.82-.01-1.49-2.01.37-2.53-.49-2.69-.94-.09-.23-.48-.94-.82-1.13-.28-.15-.68-.52-.01-.53.63-.01 1.08.58 1.23.82.72 1.21 1.87.87 2.33.66.07-.52.28-.87.51-1.07-1.78-.2-3.64-.89-3.64-3.95 0-.87.31-1.59.82-2.15-.08-.2-.36-1.02.08-2.12 0 0 .67-.21 2.2.82.64-.18 1.32-.27 2-.27.68 0 1.36.09 2 .27 1.53-1.04 2.2-.82 2.2-.82.44 1.1.16 1.92.08 2.12.51.56.82 1.27.82 2.15 0 3.07-1.87 3.75-3.65 3.95.29.25.54.73.54 1.48 0 1.07-.01 1.93-.01 2.2 0 .21.15.46.55.38A8.012 8.012 0 0 0 16 8c0-4.42-3.58-8-8-8z"/>
            </svg>
          </a>
          {meta.homepage && (
            <a href={meta.homepage} target="_blank" rel="noopener noreferrer" className="icon-btn" title="Homepage">
              <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2">
                <path d="M18 13v6a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h6"/>
                <polyline points="15 3 21 3 21 9"/><line x1="10" y1="14" x2="21" y2="3"/>
              </svg>
            </a>
          )}
        </div>
      </div>
      {meta.description && <p className="repo-description">{meta.description}</p>}
      <div className="stat-pills-row">
        <StatPill icon="★" value={meta.stars} label="Stars"/>
        <StatPill icon="⑂" value={meta.forks} label="Forks"/>
        <StatPill icon="◉" value={meta.watchers} label="Watchers"/>
        <StatPill icon="⚑" value={meta.openIssues} label="Issues"/>
      </div>
      <div className="repo-meta-grid">
        {[
          { k:'Language',  v:meta.primaryLanguage, dot:true },
          { k:'Branch',    v:meta.defaultBranch||'main', mono:true },
          { k:'License',   v:meta.license||'—' },
          { k:'Size',      v:meta.sizeKb ? `${(meta.sizeKb/1024).toFixed(1)} MB` : '—' },
          { k:'Created',   v:meta.createdAt ? new Date(meta.createdAt).toLocaleDateString() : '—' },
          { k:'Updated',   v:meta.updatedAt ? new Date(meta.updatedAt).toLocaleDateString() : '—' },
        ].map(({ k, v, dot, mono }) => (
          <div key={k} className="repo-meta-item">
            <span className="meta-key">{k}</span>
            <span className={`meta-val${mono?' mono':''}`}>
              {dot && v && <span className="lang-dot-mini" style={{ background: getLangColor('.'+v.toLowerCase()) }}/>}
              {v||'—'}
            </span>
          </div>
        ))}
      </div>
      {meta.topics?.length > 0 && (
        <div className="topics-row">
          {meta.topics.map(t => <span key={t} className="topic-chip">{t}</span>)}
        </div>
      )}
      <div className="repo-card-footer">
        <span className="analyzed-ts">
          <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2"><circle cx="12" cy="12" r="10"/><polyline points="12 6 12 12 16 14"/></svg>
          Analyzed {new Date(analyzedAt).toLocaleString()}
        </span>
      </div>
    </div>
  )
}

// ─── Section 9: Project Classification ───────────────────────────────────────

function ProjectClassificationCard({ insights, tech }) {
  if (!insights && !tech) return null
  const items = [
    { label:'Project Size',    value: insights?.estimatedProjectSize || '—',    color: sizeColor(insights?.estimatedProjectSize) },
    { label:'Maturity',        value: insights?.estimatedMaturity || '—',        color: maturityColor(insights?.estimatedMaturity) },
    { label:'Complexity',      value: insights?.estimatedComplexity || '—',      color: complexityColor(insights?.estimatedComplexity) },
    { label:'Repository Type', value: tech?.repositoryType || '—',               color: '#818cf8' },
  ]
  return (
    <div className="dash-card animate-in" style={{ '--delay':'0.05s' }}>
      <SectionHeader
        icon={<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2"><rect x="3" y="3" width="7" height="7"/><rect x="14" y="3" width="7" height="7"/><rect x="14" y="14" width="7" height="7"/><rect x="3" y="14" width="7" height="7"/></svg>}
        title="Project Classification"
        badge={tech?.confidenceScore ? `${tech.confidenceScore}% confident` : undefined}
        badgeVariant="neutral"
      />
      <div className="class-grid">
        {items.map(({ label, value, color }) => (
          <div key={label} className="class-item">
            <span className="class-label">{label}</span>
            <span className="class-value" style={{ color }}>{value}</span>
          </div>
        ))}
      </div>
    </div>
  )
}

// ─── Metric Cards ─────────────────────────────────────────────────────────────

function MetricCards({ fileCount, dirCount, totalLines, langCount }) {
  return (
    <div className="metrics-row animate-in" style={{ '--delay':'0.08s' }}>
      {[
        { v:fileCount, l:'Files', color:'#6366f1',
          icon:<svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5"><path d="M13 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V9z"/><polyline points="13 2 13 9 20 9"/></svg> },
        { v:dirCount,  l:'Directories', color:'#8b5cf6',
          icon:<svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5"><path d="M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z"/></svg> },
        { v:totalLines, l:'Lines of Code', color:'#06b6d4',
          icon:<svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5"><polyline points="16 18 22 12 16 6"/><polyline points="8 6 2 12 8 18"/></svg> },
        { v:langCount, l:'Languages', color:'#10b981',
          icon:<svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5"><circle cx="12" cy="12" r="10"/><line x1="2" y1="12" x2="22" y2="12"/><path d="M12 2a15.3 15.3 0 0 1 4 10 15.3 15.3 0 0 1-4 10 15.3 15.3 0 0 1-4-10 15.3 15.3 0 0 1 4-10z"/></svg> },
      ].map(({ v, l, color, icon }) => (
        <div key={l} className="metric-card" style={{ '--accent': color }}>
          <div className="metric-card-icon">{icon}</div>
          <div className="metric-card-value">{typeof v === 'number' ? v.toLocaleString() : v}</div>
          <div className="metric-card-label">{l}</div>
        </div>
      ))}
    </div>
  )
}

// ─── Section 3: Technology Stack ──────────────────────────────────────────────

const TECH_SECTIONS = [
  { key:'frontendFrameworks', label:'Frontend',    color:'#61dafb' },
  { key:'backendFrameworks',  label:'Backend',     color:'#6366f1' },
  { key:'buildSystems',       label:'Build',       color:'#8b5cf6' },
  { key:'packageManagers',    label:'Packages',    color:'#06b6d4' },
  { key:'testingFrameworks',  label:'Testing',     color:'#10b981' },
  { key:'ciSystems',          label:'CI/CD',       color:'#f59e0b' },
  { key:'containers',         label:'Containers',  color:'#0ea5e9' },
  { key:'cloudProviders',     label:'Cloud',       color:'#a78bfa' },
  { key:'databases',          label:'Databases',   color:'#34d399' },
]

function TechnologyCard({ tech }) {
  return (
    <div className="dash-card animate-in" style={{ '--delay':'0.1s' }}>
      <SectionHeader
        icon={<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2"><polygon points="13 2 3 14 12 14 11 22 21 10 12 10 13 2"/></svg>}
        title="Technology Stack"
        badge={tech ? `${tech.confidenceScore}% confidence` : undefined}
        badgeVariant="info"
      />
      {!tech ? (
        <NotDetected/>
      ) : (
        <div className="tech-sections">
          {TECH_SECTIONS.map(({ key, label, color }) => {
            const items = tech[key] || []
            return (
              <div key={key} className="tech-section-row">
                <span className="tech-cat-label" style={{ color }}>{label}</span>
                <div className="tech-badges-wrap">
                  {items.length > 0
                    ? items.map(t => <TechBadge key={t.name} name={t.name} confidence={t.confidence}/>)
                    : <NotDetected/>
                  }
                </div>
              </div>
            )
          })}
        </div>
      )}
    </div>
  )
}

// ─── Section 4: Repository Health ────────────────────────────────────────────

function ScoreGauge({ score, grade }) {
  const col = gradeColor(grade)
  const r = 36, circ = 2 * Math.PI * r
  const offset = circ - (score / 100) * circ
  return (
    <div className="score-gauge-wrap">
      <svg viewBox="0 0 88 88" width="88" height="88">
        <circle cx="44" cy="44" r={r} fill="none" stroke="rgba(255,255,255,0.05)" strokeWidth="8"/>
        <circle cx="44" cy="44" r={r} fill="none" stroke={col} strokeWidth="8"
          strokeDasharray={circ} strokeDashoffset={offset} strokeLinecap="round"
          transform="rotate(-90 44 44)" style={{ transition:'stroke-dashoffset 1s ease' }}/>
      </svg>
      <div className="gauge-inner">
        <span className="gauge-score" style={{ color:col }}>{score}</span>
        <span className="gauge-label">/ 100</span>
      </div>
    </div>
  )
}

const HEALTH_CATS = [
  { key:'documentation',    label:'Documentation' },
  { key:'testing',          label:'Testing' },
  { key:'ciCd',             label:'CI / CD' },
  { key:'security',         label:'Security' },
  { key:'maintainability',  label:'Maintainability' },
  { key:'configuration',    label:'Configuration' },
  { key:'projectStructure', label:'Project Structure' },
]

function HealthCategoryCard({ name, score, maxScore, grade }) {
  const pct = maxScore > 0 ? Math.round(score / maxScore * 100) : 0
  const col = healthColor(pct)
  return (
    <div className="health-cat-card">
      <div className="hcat-header">
        <span className="hcat-name">{name}</span>
        <span className="hcat-grade" style={{ color:col }}>{grade}</span>
      </div>
      <div className="hcat-score">{score}<span className="hcat-max">/{maxScore}</span></div>
      <div className="hcat-track">
        <div className="hcat-fill" style={{ width:`${pct}%`, background:col }}/>
      </div>
    </div>
  )
}

function HealthDashboard({ health }) {
  if (!health) return null
  const gc = gradeColor(health.grade)
  return (
    <div className="dash-card animate-in" style={{ '--delay':'0.15s' }}>
      <SectionHeader
        icon={<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2"><path d="M20.84 4.61a5.5 5.5 0 0 0-7.78 0L12 5.67l-1.06-1.06a5.5 5.5 0 0 0-7.78 7.78l1.06 1.06L12 21.23l7.78-7.78 1.06-1.06a5.5 5.5 0 0 0 0-7.78z"/></svg>}
        title="Repository Health"
        badge={health.grade}
        badgeVariant={health.grade === 'A' ? 'info' : health.grade === 'B' ? 'info' : health.grade === 'C' ? 'soon' : 'soon'}
      />
      <div className="health-overview">
        <ScoreGauge score={health.overallScore} grade={health.grade}/>
        <div className="health-grade-block">
          <span className="health-grade-big" style={{ color:gc }}>{health.grade}</span>
          <span className="health-grade-label">Overall Grade</span>
        </div>
        {health.qualityOverview || null}
      </div>
      <div className="health-cats-grid">
        {HEALTH_CATS.map(({ key, label }) => {
          const cat = health.categories?.[key]
          return cat ? (
            <HealthCategoryCard key={key} name={label} {...cat}/>
          ) : null
        })}
      </div>
    </div>
  )
}

// ─── Section 5 & 6: Strengths & Risks ────────────────────────────────────────

function StrengthsCard({ insights }) {
  const items = insights?.strengths || []
  return (
    <div className="dash-card animate-in" style={{ '--delay':'0.2s' }}>
      <SectionHeader
        icon={<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="#34d399" strokeWidth="2"><polyline points="20 6 9 17 4 12"/></svg>}
        title="Strengths"
        badge={items.length > 0 ? `${items.length}` : undefined}
        badgeVariant="info"
      />
      {items.length === 0
        ? <p className="not-detected">No strengths data available</p>
        : <div className="insight-list">{items.map((s,i) => <InsightItem key={i} text={s} type="strength"/>)}</div>
      }
    </div>
  )
}

function RisksCard({ insights }) {
  const items = insights?.risks || []
  return (
    <div className="dash-card animate-in" style={{ '--delay':'0.25s' }}>
      <SectionHeader
        icon={<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="#fbbf24" strokeWidth="2"><path d="M10.29 3.86L1.82 18a2 2 0 0 0 1.71 3h16.94a2 2 0 0 0 1.71-3L13.71 3.86a2 2 0 0 0-3.42 0z"/><line x1="12" y1="9" x2="12" y2="13"/><line x1="12" y1="17" x2="12.01" y2="17"/></svg>}
        title="Risks"
        badge={items.length > 0 ? `${items.length}` : undefined}
        badgeVariant="soon"
      />
      {items.length === 0
        ? <p className="not-detected">No risks detected</p>
        : <div className="insight-list">{items.map((r,i) => <InsightItem key={i} text={r} type="risk"/>)}</div>
      }
    </div>
  )
}

// ─── Section 7: Recommendations ──────────────────────────────────────────────

function RecommendationsCard({ insights }) {
  const items = insights?.suggestions || []
  return (
    <div className="dash-card animate-in" style={{ '--delay':'0.3s' }}>
      <SectionHeader
        icon={<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2"><polygon points="12 2 15.09 8.26 22 9.27 17 14.14 18.18 21.02 12 17.77 5.82 21.02 7 14.14 2 9.27 8.91 8.26 12 2"/></svg>}
        title="Recommendations"
        badge={items.length > 0 ? `${items.length}` : undefined}
        badgeVariant="roadmap"
      />
      {items.length === 0
        ? <p className="not-detected">No recommendations available</p>
        : (
          <div className="rec-list">
            {items.map((s, i) => (
              <div key={i} className="rec-card">
                <div className="rec-num">{i + 1}</div>
                <p className="rec-text">{s}</p>
              </div>
            ))}
          </div>
        )
      }
    </div>
  )
}

// ─── Section 8: Language Distribution ────────────────────────────────────────

function LanguageDistributionCard({ languages }) {
  if (!languages || Object.keys(languages).length === 0) return null
  const all = Object.entries(languages).sort((a, b) => b[1] - a[1])
  const groups = { code:[], docs:[], config:[], assets:[] }
  all.forEach(([ext, count]) => groups[extGroup(ext)].push([ext, count]))

  const GROUP_META = {
    code:   { label:'Source Code',    base: groups.code.reduce((s,[,c])=>s+c,0)   },
    docs:   { label:'Documentation',  base: groups.docs.reduce((s,[,c])=>s+c,0)   },
    config: { label:'Configuration',  base: groups.config.reduce((s,[,c])=>s+c,0) },
    assets: { label:'Assets & Media', base: groups.assets.reduce((s,[,c])=>s+c,0) },
  }
  const total = all.reduce((s,[,c])=>s+c, 0)

  const LangRow = ({ ext, count, base, asset }) => {
    const pct = base > 0 ? Math.round((count/base)*100) : 0
    const col = asset ? '#4b5563' : getLangColor(ext)
    return (
      <div className={`lang-row-v2${asset ? ' asset-row':''}`}>
        <div className="lang-name-v2">
          <span className="lang-dot-sm" style={{ background:col }}/>
          <code>{ext}</code>
        </div>
        <div className="lang-bar-track">
          <div className="lang-bar-fill" style={{ width:`${Math.max(pct,1)}%`, background:col }}/>
        </div>
        <span className="lang-stats">{count} · {pct}%</span>
      </div>
    )
  }

  return (
    <div className="dash-card animate-in" style={{ '--delay':'0.15s' }}>
      <SectionHeader
        icon={<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2"><circle cx="12" cy="12" r="10"/><line x1="2" y1="12" x2="22" y2="12"/><path d="M12 2a15.3 15.3 0 0 1 4 10 15.3 15.3 0 0 1-4 10 15.3 15.3 0 0 1-4-10 15.3 15.3 0 0 1 4-10z"/></svg>}
        title="Language Distribution"
        badge={`${total} files`}
        badgeVariant="neutral"
      />
      {['code','docs','config','assets'].map(g => {
        const rows = groups[g]
        if (rows.length === 0) return null
        const { label, base } = GROUP_META[g]
        return (
          <div key={g} className="lang-group">
            <div className="lang-group-title">{label}</div>
            {rows.slice(0,10).map(([ext,count]) => (
              <LangRow key={ext} ext={ext} count={count} base={base} asset={g==='assets'}/>
            ))}
          </div>
        )
      })}
    </div>
  )
}

// ─── Loading Skeleton ─────────────────────────────────────────────────────────

function DashboardSkeleton() {
  return (
    <div className="dash-skeleton">
      <div className="skel-card big">
        <div className="skel-avatar"/>
        <div className="skel-lines">
          <div className="skel-line w60"/><div className="skel-line w80"/><div className="skel-line w40"/>
        </div>
      </div>
      <div className="skel-metrics">
        {[1,2,3,4].map(i => <div key={i} className="skel-card skel-metric"><div className="skel-line w50"/></div>)}
      </div>
      <div className="skel-row">
        <div className="skel-card half"><div className="skel-line w40"/><div className="skel-line w90"/><div className="skel-line w70"/></div>
        <div className="skel-card half"><div className="skel-line w40"/><div className="skel-line w80"/><div className="skel-line w60"/></div>
      </div>
    </div>
  )
}

// ─── Dashboard Orchestrator ───────────────────────────────────────────────────

function AnalysisDashboard({ analysis }) {
  const meta      = analysis.metadata
  const tech      = analysis.technologyAnalysis
  const health    = analysis.repositoryHealth
  const insights  = analysis.repositoryInsights
  const langCount = Object.keys(analysis.languages).length

  return (
    <div className="analysis-dashboard">
      {/* Section 2: Executive Summary */}
      <ExecutiveSummaryCard insights={insights}/>

      {/* Section 1: Repository Overview */}
      {meta && <RepositoryOverviewCard meta={meta} analyzedAt={analysis.analyzedAt}/>}

      {/* Metrics row */}
      <MetricCards
        fileCount={analysis.fileCount}
        dirCount={analysis.dirCount}
        totalLines={analysis.totalLines}
        langCount={langCount}
      />

      {/* Section 9: Project Classification */}
      <ProjectClassificationCard insights={insights} tech={tech}/>

      {/* Section 3 + 8: Technology + Language (side by side) */}
      <div className="two-col-grid">
        <TechnologyCard tech={tech}/>
        <LanguageDistributionCard languages={analysis.languages}/>
      </div>

      {/* Section 4: Health Dashboard */}
      <HealthDashboard health={health}/>

      {/* Section 5 + 6: Strengths + Risks */}
      <div className="two-col-grid">
        <StrengthsCard insights={insights}/>
        <RisksCard insights={insights}/>
      </div>

      {/* Section 7: Recommendations */}
      <RecommendationsCard insights={insights}/>
    </div>
  )
}

// ─── Root App ─────────────────────────────────────────────────────────────────

export default function App() {
  const [url, setUrl] = useState('')
  const [phase, setPhase] = useState('idle')
  const [statusMsg, setStatusMsg] = useState('')
  const [analysis, setAnalysis] = useState(null)

  const poll = useCallback(async (jobId) => {
    setPhase('polling')
    const interval = setInterval(async () => {
      try {
        const r = await fetch(`${API}/jobs/${jobId}`)
        const json = await r.json()
        const status = json.data?.status ?? 'UNKNOWN'
        setStatusMsg(`Analyzing… — ${status}`)
        if (status === 'COMPLETED') {
          clearInterval(interval)
          const ar = await fetch(`${API}/analysis/${jobId}`)
          const aj = await ar.json()
          if (aj.success) { setAnalysis(aj.data); setPhase('done') }
          else { setStatusMsg('Analysis unavailable: ' + (aj.message ?? '')); setPhase('error') }
        } else if (status === 'FAILED') {
          clearInterval(interval); setStatusMsg('Analysis failed.'); setPhase('error')
        }
      } catch (err) {
        clearInterval(interval); setStatusMsg('Error: ' + err.message); setPhase('error')
      }
    }, 2000)
  }, [])

  const handleSubmit = async (e) => {
    e.preventDefault()
    if (!url.trim()) return
    setPhase('submitting'); setAnalysis(null); setStatusMsg('Submitting…')
    try {
      const r = await fetch(`${API}/repositories`, {
        method:'POST', headers:{'Content-Type':'application/json'},
        body: JSON.stringify({ repositoryUrl: url.trim() })
      })
      const json = await r.json()
      if (!json.success) { setStatusMsg(json.message ?? 'Failed'); setPhase('error'); return }
      poll(json.data.jobId)
    } catch (err) { setStatusMsg('Request failed: ' + err.message); setPhase('error') }
  }

  return (
    <div className="app-wrapper">
      <div className="container">
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
                <path d="M8 0C3.58 0 0 3.58 0 8c0 3.54 2.29 6.53 5.47 7.59.4.07.55-.17.55-.38 0-.19-.01-.82-.01-1.49-2.01.37-2.53-.49-2.69-.94-.09-.23-.48-.94-.82-1.13-.28-.15-.68-.52-.01-.53.63-.01 1.08.58 1.23.82.72 1.21 1.87.87 2.33.66.07-.52.28-.87.51-1.07-1.78-.2-3.64-.89-3.64-3.95 0-.87.31-1.59.82-2.15-.08-.2-.36-1.02.08-2.12 0 0 .67-.21 2.2.82.64-.18 1.32-.27 2-.27.68 0 1.36.09 2 .27 1.53-1.04 2.2-.82 2.2-.82.44 1.1.16 1.92.08 2.12.51.56.82 1.27.82 2.15 0 3.07-1.87 3.75-3.65 3.95.29.25.54.73.54 1.48 0 1.07-.01 1.93-.01 2.2 0 .21.15.46.55.38A8.012 8.012 0 0 0 16 8c0-4.42-3.58-8-8-8z"/>
              </svg>
              View Source
            </a>
          </div>
          <p className="header-subtitle">Repository intelligence platform — analyze any public GitHub repository</p>
        </header>

        <section className="hero">
          <form className="form-premium" onSubmit={handleSubmit}>
            <input type="text" className="url-input-premium"
              placeholder="https://github.com/user/repo"
              value={url} onChange={e => setUrl(e.target.value)}
              disabled={phase === 'submitting' || phase === 'polling'}/>
            <button type="submit" className="btn-primary"
              disabled={phase === 'submitting' || phase === 'polling' || !url.trim()}>
              {phase === 'submitting' || phase === 'polling' ? 'Analyzing…' : 'Analyze'}
            </button>
          </form>
          <p className="hero-caption">Filesystem analysis · GitHub metadata · Technology detection · Health scoring · AI-free insights</p>
        </section>

        {statusMsg && (
          <div className={`status-premium${phase==='error'?' status-error':phase==='done'?' status-done':''}`}>
            {phase === 'polling' && <span className="spinner"/>}
            {statusMsg}
          </div>
        )}

        {phase === 'polling' && <DashboardSkeleton/>}
        {phase === 'done' && analysis && <AnalysisDashboard analysis={analysis}/>}
      </div>

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
                <span>C++20</span><span>Drogon</span><span>React</span><span>Docker</span><span>GitHub Actions</span>
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
              <div><div className="dev-name">Kartick Kumar Ghosh</div><div className="dev-role">Software Engineer</div></div>
              <div className="specialization">
                <span className="spec-label">Specializing in</span>
                <div className="spec-items">
                  <span>Modern C++</span><span>Backend Engineering</span><span>Distributed Systems</span><span>System Design</span>
                </div>
              </div>
              <div className="social-links">
                <a href="https://github.com/gavinspet" target="_blank" rel="noopener noreferrer" className="social-btn">
                  <svg width="20" height="20" viewBox="0 0 16 16" fill="currentColor"><path d="M8 0C3.58 0 0 3.58 0 8c0 3.54 2.29 6.53 5.47 7.59.4.07.55-.17.55-.38 0-.19-.01-.82-.01-1.49-2.01.37-2.53-.49-2.69-.94-.09-.23-.48-.94-.82-1.13-.28-.15-.68-.52-.01-.53.63-.01 1.08.58 1.23.82.72 1.21 1.87.87 2.33.66.07-.52.28-.87.51-1.07-1.78-.2-3.64-.89-3.64-3.95 0-.87.31-1.59.82-2.15-.08-.2-.36-1.02.08-2.12 0 0 .67-.21 2.2.82.64-.18 1.32-.27 2-.27.68 0 1.36.09 2 .27 1.53-1.04 2.2-.82 2.2-.82.44 1.1.16 1.92.08 2.12.51.56.82 1.27.82 2.15 0 3.07-1.87 3.75-3.65 3.95.29.25.54.73.54 1.48 0 1.07-.01 1.93-.01 2.2 0 .21.15.46.55.38A8.012 8.012 0 0 0 16 8c0-4.42-3.58-8-8-8z"/></svg>
                </a>
                <a href="mailto:kartick.ghosh.dev@gmail.com" className="social-btn">
                  <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2"><path d="M4 4h16c1.1 0 2 .9 2 2v12c0 1.1-.9 2-2 2H4c-1.1 0-2-.9-2-2V6c0-1.1.9-2 2-2z"/><polyline points="22,6 12,13 2,6"/></svg>
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
