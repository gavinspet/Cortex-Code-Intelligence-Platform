import { useState, useCallback } from 'react'

const getApiUrl = () => {
  const envUrl = import.meta.env.VITE_API_URL
  if (envUrl) return envUrl
  if (typeof window !== 'undefined' && window.location.hostname.includes('vercel.app'))
    return 'https://cortex-code-intelligence-platform.onrender.com'
  return 'http://localhost:8080'
}
const API = getApiUrl()

// ─── Language colour map (GitHub Linguist inspired) ─────────────────────────

const LANG_COLORS = {
  '.js':'#f7df1e', '.jsx':'#61dafb', '.ts':'#3178c6', '.tsx':'#61dafb',
  '.py':'#3572a5', '.cpp':'#f34b7d', '.cc':'#f34b7d', '.cxx':'#f34b7d',
  '.c':'#555555', '.h':'#6e4c13', '.hpp':'#f34b7d', '.java':'#b07219',
  '.go':'#00add8', '.rs':'#dea584', '.rb':'#701516', '.php':'#4f5d95',
  '.cs':'#178600', '.swift':'#f05138', '.kt':'#7f52ff', '.vue':'#41b883',
  '.svelte':'#ff3e00', '.html':'#e34c26', '.css':'#563d7c', '.scss':'#c6538c',
  '.sass':'#a53b70', '.md':'#083fa1', '.json':'#292929', '.yml':'#cb171e',
  '.yaml':'#cb171e', '.sh':'#89e051', '.bash':'#89e051', '.sql':'#e38c00',
}
const ASSET_EXTS = new Set(['.png','.jpg','.jpeg','.gif','.svg','.ico',
  '.mp3','.mp4','.wav','.ogg','.woff','.woff2','.ttf','.otf','.eot',
  '.webp','.bmp','.tiff'])
const CODE_EXTS = new Set(['.js','.jsx','.ts','.tsx','.py','.java','.cpp',
  '.cc','.cxx','.c','.h','.hpp','.go','.rs','.rb','.php','.cs','.swift',
  '.kt','.vue','.svelte','.scala','.r','.sh','.bash','.zsh','.css','.scss',
  '.sass','.html','.xml','.json','.yaml','.yml','.toml','.md','.sql','.elm',
  '.ex','.exs','.dart','.lua','.clj'])

const getLangColor = ext => LANG_COLORS[ext] || '#6366f1'

// ─── Tech stack detection from file extensions + primary language ────────────

function detectTechStack(languages = {}, primaryLanguage = '') {
  const exts = Object.keys(languages)
  const has = (...e) => e.some(x => exts.includes(x))
  const lang = primaryLanguage || ''

  if (lang === 'JavaScript' || lang === 'TypeScript' || has('.jsx', '.tsx'))
    return { framework: has('.jsx','.tsx') ? 'React / JSX' : 'Node.js', buildSystem: 'Vite / Webpack', packageManager: 'npm / yarn', testing: 'Detection coming soon', cicd: 'Detection coming soon', container: 'Detection coming soon' }
  if (lang === 'Python' || has('.py'))
    return { framework: 'Python', buildSystem: 'setuptools / build', packageManager: 'pip / Poetry', testing: 'Detection coming soon', cicd: 'Detection coming soon', container: 'Detection coming soon' }
  if (lang === 'C++' || lang === 'C' || has('.cpp', '.cc', '.hpp'))
    return { framework: 'C++20', buildSystem: 'CMake / Make', packageManager: 'System packages', testing: 'Detection coming soon', cicd: 'Detection coming soon', container: 'Detection coming soon' }
  if (lang === 'Java' || has('.java'))
    return { framework: 'Java', buildSystem: 'Maven / Gradle', packageManager: 'Maven / Gradle', testing: 'Detection coming soon', cicd: 'Detection coming soon', container: 'Detection coming soon' }
  if (lang === 'Go' || has('.go'))
    return { framework: 'Go', buildSystem: 'go build', packageManager: 'Go Modules', testing: 'Detection coming soon', cicd: 'Detection coming soon', container: 'Detection coming soon' }
  if (lang === 'Rust' || has('.rs'))
    return { framework: 'Rust', buildSystem: 'Cargo', packageManager: 'Cargo', testing: 'Detection coming soon', cicd: 'Detection coming soon', container: 'Detection coming soon' }
  if (lang === 'Ruby' || has('.rb'))
    return { framework: 'Ruby', buildSystem: 'Rake', packageManager: 'Bundler / Gem', testing: 'Detection coming soon', cicd: 'Detection coming soon', container: 'Detection coming soon' }
  return { framework: 'Detection coming soon', buildSystem: 'Detection coming soon', packageManager: 'Detection coming soon', testing: 'Detection coming soon', cicd: 'Detection coming soon', container: 'Detection coming soon' }
}

// ─── Reusable primitives ─────────────────────────────────────────────────────

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

// ─── 1. Repository Overview Card ─────────────────────────────────────────────

function RepositoryOverviewCard({ meta, analyzedAt }) {
  if (!meta) return null
  return (
    <div className="dash-card repo-overview animate-in">
      <div className="repo-top">
        <div className="repo-identity">
          {meta.ownerAvatarUrl
            ? <img src={meta.ownerAvatarUrl} alt={meta.owner} className="owner-avatar" />
            : <div className="owner-avatar-placeholder">{(meta.owner || '?')[0].toUpperCase()}</div>
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
                <polyline points="15 3 21 3 21 9"/>
                <line x1="10" y1="14" x2="21" y2="3"/>
              </svg>
            </a>
          )}
        </div>
      </div>

      {meta.description && <p className="repo-description">{meta.description}</p>}

      <div className="stat-pills-row">
        <StatPill icon="★" value={meta.stars} label="Stars" />
        <StatPill icon="⑂" value={meta.forks} label="Forks" />
        <StatPill icon="◉" value={meta.watchers} label="Watchers" />
        <StatPill icon="⚑" value={meta.openIssues} label="Issues" />
      </div>

      <div className="repo-meta-grid">
        {[
          { k: 'Language', v: meta.primaryLanguage, dot: true },
          { k: 'Branch', v: meta.defaultBranch || 'main', mono: true },
          { k: 'License', v: meta.license || '—' },
          { k: 'Size', v: meta.sizeKb ? `${(meta.sizeKb / 1024).toFixed(1)} MB` : '—' },
          { k: 'Created', v: meta.createdAt ? new Date(meta.createdAt).toLocaleDateString() : '—' },
          { k: 'Updated', v: meta.updatedAt ? new Date(meta.updatedAt).toLocaleDateString() : '—' },
        ].map(({ k, v, dot, mono }) => (
          <div key={k} className="repo-meta-item">
            <span className="meta-key">{k}</span>
            <span className={`meta-val ${mono ? 'mono' : ''}`}>
              {dot && v && <span className="lang-dot-mini" style={{ background: getLangColor('.' + v.toLowerCase()) }} />}
              {v || '—'}
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

// ─── 2. Metric Cards ─────────────────────────────────────────────────────────

function MetricCards({ fileCount, dirCount, totalLines, langCount }) {
  return (
    <div className="metrics-row animate-in" style={{ '--delay': '0.05s' }}>
      {[
        { v: fileCount, l: 'Files', color: '#6366f1', icon: <svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5"><path d="M13 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V9z"/><polyline points="13 2 13 9 20 9"/></svg> },
        { v: dirCount, l: 'Directories', color: '#8b5cf6', icon: <svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5"><path d="M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z"/></svg> },
        { v: totalLines, l: 'Lines of Code', color: '#06b6d4', icon: <svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5"><polyline points="16 18 22 12 16 6"/><polyline points="8 6 2 12 8 18"/></svg> },
        { v: langCount, l: 'Languages', color: '#10b981', icon: <svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5"><circle cx="12" cy="12" r="10"/><line x1="2" y1="12" x2="22" y2="12"/><path d="M12 2a15.3 15.3 0 0 1 4 10 15.3 15.3 0 0 1-4 10 15.3 15.3 0 0 1-4-10 15.3 15.3 0 0 1 4-10z"/></svg> },
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

// ─── 3. Technology Stack Card ─────────────────────────────────────────────────

const TECH_META = {
  framework:      { label: 'Framework / Runtime', icon: <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2"><polygon points="13 2 3 14 12 14 11 22 21 10 12 10 13 2"/></svg> },
  buildSystem:    { label: 'Build System', icon: <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2"><circle cx="12" cy="12" r="3"/><path d="M19.07 4.93a10 10 0 0 1 0 14.14M4.93 4.93a10 10 0 0 0 0 14.14"/></svg> },
  packageManager: { label: 'Package Manager', icon: <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2"><path d="M21 16V8a2 2 0 0 0-1-1.73l-7-4a2 2 0 0 0-2 0l-7 4A2 2 0 0 0 3 8v8a2 2 0 0 0 1 1.73l7 4a2 2 0 0 0 2 0l7-4A2 2 0 0 0 21 16z"/></svg> },
  testing:        { label: 'Testing Framework', icon: <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2"><polyline points="20 6 9 17 4 12"/></svg> },
  cicd:           { label: 'CI / CD', icon: <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2"><polyline points="16 3 21 3 21 8"/><line x1="4" y1="20" x2="21" y2="3"/><polyline points="21 16 21 21 16 21"/><line x1="15" y1="15" x2="21" y2="21"/></svg> },
  container:      { label: 'Container', icon: <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2"><rect x="2" y="7" width="20" height="14" rx="2"/><path d="M16 7V5a2 2 0 0 0-4 0v2M12 12v5M8 12v5"/></svg> },
}

function TechnologyCard({ stack }) {
  return (
    <div className="dash-card animate-in" style={{ '--delay': '0.1s' }}>
      <SectionHeader icon={<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2"><polygon points="13 2 3 14 12 14 11 22 21 10 12 10 13 2"/></svg>} title="Technology Stack" badge="Auto-detected" badgeVariant="info" />
      <div className="tech-list">
        {Object.entries(TECH_META).map(([key, { label, icon }]) => {
          const val = stack[key]
          const isComing = val === 'Detection coming soon'
          return (
            <div key={key} className="tech-row">
              <div className="tech-row-icon">{icon}</div>
              <span className="tech-row-label">{label}</span>
              <span className={`tech-row-value ${isComing ? 'coming' : 'detected'}`}>{val}</span>
            </div>
          )
        })}
      </div>
    </div>
  )
}

// ─── 5. Language Distribution Chart ──────────────────────────────────────────

function LanguageChart({ languages }) {
  if (!languages || Object.keys(languages).length === 0) return null
  const all = Object.entries(languages).sort((a, b) => b[1] - a[1])
  const code  = all.filter(([e]) => CODE_EXTS.has(e))
  const assets = all.filter(([e]) => ASSET_EXTS.has(e))
  const other  = all.filter(([e]) => !CODE_EXTS.has(e) && !ASSET_EXTS.has(e))
  const total = all.reduce((s, [, c]) => s + c, 0)
  const codeTotal = code.reduce((s, [, c]) => s + c, 0)

  const LangRow = ({ ext, count, base, assetStyle }) => {
    const pct = base > 0 ? Math.round((count / base) * 100) : 0
    return (
      <div className={`lang-row-v2 ${assetStyle ? 'asset-row' : ''}`}>
        <div className="lang-name-v2">
          <span className="lang-dot-sm" style={{ background: assetStyle ? '#4b5563' : getLangColor(ext) }} />
          <code>{ext}</code>
        </div>
        <div className="lang-bar-track">
          <div className="lang-bar-fill" style={{ width: `${Math.max(pct, 1)}%`, background: assetStyle ? '#4b5563' : getLangColor(ext) }} />
        </div>
        <span className="lang-stats">{count} · {pct}%</span>
      </div>
    )
  }

  return (
    <div className="dash-card animate-in" style={{ '--delay': '0.15s' }}>
      <SectionHeader icon={<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2"><circle cx="12" cy="12" r="10"/><line x1="2" y1="12" x2="22" y2="12"/><path d="M12 2a15.3 15.3 0 0 1 4 10 15.3 15.3 0 0 1-4 10 15.3 15.3 0 0 1-4-10 15.3 15.3 0 0 1 4-10z"/></svg>} title="Language Distribution" badge={`${total} files`} badgeVariant="neutral" />
      {code.length > 0 && (
        <div className="lang-group">
          <div className="lang-group-title">Source Code</div>
          {code.slice(0, 12).map(([e, c]) => <LangRow key={e} ext={e} count={c} base={codeTotal} />)}
        </div>
      )}
      {assets.length > 0 && (
        <div className="lang-group">
          <div className="lang-group-title">Assets &amp; Media</div>
          {assets.map(([e, c]) => <LangRow key={e} ext={e} count={c} base={total} assetStyle />)}
        </div>
      )}
      {other.length > 0 && (
        <div className="lang-group">
          <div className="lang-group-title">Other</div>
          {other.map(([e, c]) => <LangRow key={e} ext={e} count={c} base={total} />)}
        </div>
      )}
    </div>
  )
}

// ─── 4. Dependency Preview ────────────────────────────────────────────────────

function DependencyCard() {
  return (
    <div className="dash-card placeholder-card animate-in" style={{ '--delay': '0.2s' }}>
      <SectionHeader icon={<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2"><path d="M21 16V8a2 2 0 0 0-1-1.73l-7-4a2 2 0 0 0-2 0l-7 4A2 2 0 0 0 3 8v8a2 2 0 0 0 1 1.73l7 4a2 2 0 0 0 2 0l7-4A2 2 0 0 0 21 16z"/></svg>} title="Dependency Analysis" badge="V1.1" badgeVariant="roadmap" />
      <div className="placeholder-body">
        <div className="placeholder-items">
          {['Direct Dependencies', 'Transitive Graph', 'Vulnerability Scan', 'Version Analysis'].map(f => (
            <div key={f} className="placeholder-row">
              <span className="placeholder-dot" />
              <span>{f}</span>
            </div>
          ))}
        </div>
        <p className="placeholder-note">Full dependency graph, outdated packages and vulnerability detection — coming in V1.1</p>
      </div>
    </div>
  )
}

// ─── 6. Repository Health ─────────────────────────────────────────────────────

function RepositoryHealthCard() {
  const items = [
    { label: 'Architecture', icon: <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2"><rect x="3" y="3" width="7" height="7"/><rect x="14" y="3" width="7" height="7"/><rect x="14" y="14" width="7" height="7"/><rect x="3" y="14" width="7" height="7"/></svg> },
    { label: 'Documentation', icon: <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/><polyline points="14 2 14 8 20 8"/><line x1="16" y1="13" x2="8" y2="13"/><line x1="16" y1="17" x2="8" y2="17"/></svg> },
    { label: 'Test Coverage', icon: <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2"><polyline points="20 6 9 17 4 12"/></svg> },
    { label: 'Complexity', icon: <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2"><polyline points="22 12 18 12 15 21 9 3 6 12 2 12"/></svg> },
  ]
  return (
    <div className="dash-card placeholder-card animate-in" style={{ '--delay': '0.25s' }}>
      <SectionHeader icon={<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2"><path d="M20.84 4.61a5.5 5.5 0 0 0-7.78 0L12 5.67l-1.06-1.06a5.5 5.5 0 0 0-7.78 7.78l1.06 1.06L12 21.23l7.78-7.78 1.06-1.06a5.5 5.5 0 0 0 0-7.78z"/></svg>} title="Repository Health" badge="Coming soon" badgeVariant="soon" />
      <div className="health-grid">
        {items.map(({ label, icon }) => (
          <div key={label} className="health-item">
            <div className="health-item-header">
              <span className="health-item-icon">{icon}</span>
              <span className="health-item-label">{label}</span>
            </div>
            <div className="health-bar-track">
              <div className="health-bar-shimmer" />
            </div>
            <span className="health-item-status">Analysis coming soon</span>
          </div>
        ))}
      </div>
    </div>
  )
}

// ─── 7. Recommendations Card ──────────────────────────────────────────────────

function RecommendationsCard() {
  return (
    <div className="dash-card placeholder-card animate-in" style={{ '--delay': '0.3s' }}>
      <SectionHeader icon={<svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2"><polygon points="12 2 15.09 8.26 22 9.27 17 14.14 18.18 21.02 12 17.77 5.82 21.02 7 14.14 2 9.27 8.91 8.26 12 2"/></svg>} title="AI Recommendations" badge="V1.2" badgeVariant="roadmap" />
      <div className="placeholder-body ai-placeholder">
        <div className="ai-icon-wrap">
          <svg width="32" height="32" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5">
            <polygon points="12 2 15.09 8.26 22 9.27 17 14.14 18.18 21.02 12 17.77 5.82 21.02 7 14.14 2 9.27 8.91 8.26 12 2"/>
          </svg>
        </div>
        <p className="placeholder-note">AI-powered code quality, security insights, performance tips and best practices — available in V1.2</p>
        <div className="placeholder-items">
          {['Code Quality', 'Security Insights', 'Performance Tips', 'Best Practices'].map(f => (
            <div key={f} className="placeholder-row">
              <span className="placeholder-dot ai-dot" />
              <span>{f}</span>
            </div>
          ))}
        </div>
      </div>
    </div>
  )
}

// ─── Loading skeleton ─────────────────────────────────────────────────────────

function DashboardSkeleton() {
  return (
    <div className="dash-skeleton">
      <div className="skel-card big">
        <div className="skel-avatar" />
        <div className="skel-lines">
          <div className="skel-line w60" />
          <div className="skel-line w80" />
          <div className="skel-line w40" />
        </div>
      </div>
      <div className="skel-metrics">
        {[1,2,3,4].map(i => <div key={i} className="skel-card skel-metric"><div className="skel-line w50" /></div>)}
      </div>
      <div className="skel-row">
        <div className="skel-card half">
          <div className="skel-line w40" />
          <div className="skel-line w90" />
          <div className="skel-line w70" />
        </div>
        <div className="skel-card half">
          <div className="skel-line w40" />
          <div className="skel-line w80" />
          <div className="skel-line w60" />
        </div>
      </div>
    </div>
  )
}

// ─── Dashboard orchestrator ───────────────────────────────────────────────────

function AnalysisDashboard({ analysis }) {
  const meta = analysis.metadata
  const stack = detectTechStack(analysis.languages, meta?.primaryLanguage)
  const langCount = Object.keys(analysis.languages).length

  return (
    <div className="analysis-dashboard">
      {meta && <RepositoryOverviewCard meta={meta} analyzedAt={analysis.analyzedAt} />}

      <MetricCards
        fileCount={analysis.fileCount}
        dirCount={analysis.dirCount}
        totalLines={analysis.totalLines}
        langCount={langCount}
      />

      <div className="two-col-grid">
        <TechnologyCard stack={stack} />
        <LanguageChart languages={analysis.languages} />
      </div>

      <div className="two-col-grid">
        <DependencyCard />
        <RepositoryHealthCard />
      </div>

      <RecommendationsCard />
    </div>
  )
}

// ─── Root App ─────────────────────────────────────────────────────────────────

export default function App() {
  const [url, setUrl] = useState('')
  const [phase, setPhase] = useState('idle')
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
          const ar = await fetch(`${API}/analysis/${jobId}`)
          const aj = await ar.json()
          if (aj.success) { setAnalysis(aj.data); setPhase('done') }
          else { setStatusMsg('Analysis unavailable: ' + (aj.message ?? '')); setPhase('error') }
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
      if (!json.success) { setStatusMsg(json.message ?? 'Submission failed'); setPhase('error'); return }
      poll(json.data.jobId)
    } catch (err) {
      setStatusMsg('Request failed: ' + err.message)
      setPhase('error')
    }
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
          <p className="header-subtitle">Analyze any public GitHub repository</p>
        </header>

        <section className="hero">
          <form className="form-premium" onSubmit={handleSubmit}>
            <input type="text" className="url-input-premium"
              placeholder="https://github.com/user/repo"
              value={url} onChange={e => setUrl(e.target.value)}
              disabled={phase === 'submitting' || phase === 'polling'} />
            <button type="submit" className="btn-primary"
              disabled={phase === 'submitting' || phase === 'polling' || !url.trim()}>
              {phase === 'submitting' || phase === 'polling' ? 'Analyzing…' : 'Analyze'}
            </button>
          </form>
          <p className="hero-caption">Analyze public GitHub repositories using a high-performance C++20 backend.</p>
        </section>

        {statusMsg && (
          <div className={`status-premium ${phase === 'error' ? 'status-error' : ''} ${phase === 'done' ? 'status-done' : ''}`}>
            {phase === 'polling' && <span className="spinner" />}
            {statusMsg}
          </div>
        )}

        {phase === 'polling' && (
          <DashboardSkeleton />
        )}

        {phase === 'done' && analysis && <AnalysisDashboard analysis={analysis} />}
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
                <a href="https://github.com/gavinspet" target="_blank" rel="noopener noreferrer" className="social-btn" title="GitHub">
                  <svg width="20" height="20" viewBox="0 0 16 16" fill="currentColor"><path d="M8 0C3.58 0 0 3.58 0 8c0 3.54 2.29 6.53 5.47 7.59.4.07.55-.17.55-.38 0-.19-.01-.82-.01-1.49-2.01.37-2.53-.49-2.69-.94-.09-.23-.48-.94-.82-1.13-.28-.15-.68-.52-.01-.53.63-.01 1.08.58 1.23.82.72 1.21 1.87.87 2.33.66.07-.52.28-.87.51-1.07-1.78-.2-3.64-.89-3.64-3.95 0-.87.31-1.59.82-2.15-.08-.2-.36-1.02.08-2.12 0 0 .67-.21 2.2.82.64-.18 1.32-.27 2-.27.68 0 1.36.09 2 .27 1.53-1.04 2.2-.82 2.2-.82.44 1.1.16 1.92.08 2.12.51.56.82 1.27.82 2.15 0 3.07-1.87 3.75-3.65 3.95.29.25.54.73.54 1.48 0 1.07-.01 1.93-.01 2.2 0 .21.15.46.55.38A8.012 8.012 0 0 0 16 8c0-4.42-3.58-8-8-8z"/></svg>
                </a>
                <a href="mailto:kartick.ghosh.dev@gmail.com" className="social-btn" title="Email">
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

