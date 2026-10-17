import { useEffect, useMemo, useState } from 'react'
import { formatDate, formatPercent, parseRepoFromUrl } from '../lib/format.js'
import { DistributionBar, GradeBadge, Meter, ScoreGauge, buildExtensionRows, fileLabel, topSegments } from './ReportCharts.jsx'
import { LayerIcon } from './ArchitectureVisuals.jsx'
import { cn } from './ui.jsx'

// `frameworks` is an aggregate of the frontend/backend groups; only its leftovers are listed.
const TECH_GROUPS = [
  ['frontendFrameworks', 'Frontend frameworks'],
  ['backendFrameworks', 'Backend frameworks'],
  ['frameworks', 'Other frameworks'],
  ['buildSystems', 'Build tooling'],
  ['packageManagers', 'Package managers'],
  ['testingFrameworks', 'Testing'],
  ['ciSystems', 'CI / CD'],
  ['containers', 'Containers'],
  ['cloudProviders', 'Cloud'],
  ['databases', 'Databases'],
]

// Tabs are only shown when the backend returned the corresponding section.
const REPORT_TABS = [
  ['overview', 'Overview', () => true],
  ['health', 'Health', (analysis) => Boolean(analysis.repositoryHealth)],
  ['technology', 'Technology', (analysis) => Boolean(analysis.technologyAnalysis)],
  ['languages', 'File types', () => true],
  ['insights', 'Insights', (analysis) => Boolean(analysis.repositoryInsights)],
]

const HEALTH_CATEGORY_LABELS = {
  documentation: 'Documentation',
  testing: 'Testing',
  ciCd: 'CI / CD',
  security: 'Security',
  maintainability: 'Maintainability',
  configuration: 'Configuration',
  projectStructure: 'Project structure',
}

const DOCUMENTATION_LABELS = {
  readme: 'README',
  license: 'License',
  changelog: 'Changelog',
  contributing: 'Contributing guide',
  security: 'Security policy',
  codeOfConduct: 'Code of conduct',
}

const OVERVIEW_EXTENSION_LIMIT = 5
const FILE_TYPES_EXTENSION_LIMIT = 8
const OVERVIEW_FINDING_LIMIT = 2

export function resolveRepositoryIdentity(meta, submittedUrl) {
  const fromUrl = parseRepoFromUrl(submittedUrl)
  if (meta?.fullName) {
    return {
      label: meta.fullName,
      owner: meta.owner || meta.fullName.split('/')[0],
      name: meta.name || meta.fullName.split('/')[1],
      link: `https://github.com/${meta.fullName}`,
    }
  }
  if (meta?.owner && meta?.name) {
    return {
      label: `${meta.owner}/${meta.name}`,
      owner: meta.owner,
      name: meta.name,
      link: `https://github.com/${meta.owner}/${meta.name}`,
    }
  }
  if (fromUrl) {
    return {
      label: fromUrl.fullName,
      owner: fromUrl.owner,
      name: fromUrl.name,
      link: fromUrl.githubLink,
    }
  }
  return { label: 'Repository analysis', owner: 'Unknown', name: 'Repository', link: submittedUrl || null }
}

// Fallback label for category keys the backend may add later, e.g. "projectStructure" -> "Project structure".
function humanizeKey(key) {
  const words = key.replace(/([a-z0-9])([A-Z])/g, '$1 $2').replace(/[_-]+/g, ' ').trim().toLowerCase()
  return words.charAt(0).toUpperCase() + words.slice(1)
}

// Uses whichever categories the backend returned, sorted weakest first by share of their maximum.
function buildCategoryRows(categories) {
  return Object.entries(categories || {})
    .filter(([, item]) => item && typeof item.score === 'number' && typeof item.maxScore === 'number' && item.maxScore > 0)
    .map(([key, item]) => ({ key, label: HEALTH_CATEGORY_LABELS[key] || humanizeKey(key), score: item.score, maxScore: item.maxScore, grade: item.grade, ratio: item.score / item.maxScore }))
    .sort((a, b) => a.ratio - b.ratio)
}

function buildTechGroups(tech) {
  const specificNames = new Set([...(tech.frontendFrameworks || []), ...(tech.backendFrameworks || [])].map((item) => item?.name))
  return TECH_GROUPS.map(([key, label]) => {
    const raw = Array.isArray(tech[key]) ? tech[key] : null
    const items = (raw || []).filter((item) => item?.name && !(key === 'frameworks' && specificNames.has(item.name)))
    return { key, label, items, reportedEmpty: raw !== null && raw.length === 0 }
  })
}

function uniqueTechnologyNames(groups) {
  const names = []
  for (const group of groups) {
    for (const item of group.items) {
      if (!names.includes(item.name)) names.push(item.name)
    }
  }
  return names
}

function toList(items) {
  return Array.isArray(items) ? items.filter(Boolean) : []
}

function ReportHeading({ id, title, description }) {
  return (
    <div className="section-heading">
      <h2 id={id}>{title}</h2>
      {description ? <p>{description}</p> : null}
    </div>
  )
}

function TabLink({ tab, onNavigate, children }) {
  if (!onNavigate?.canNavigate(tab)) return null
  return <button type="button" className="link-button card-link" onClick={() => onNavigate.go(tab)}>{children}</button>
}

const FINDING_ICONS = {
  warning: <path d="M12 4 21 20H3L12 4Zm0 6v4m0 3h.01" />,
  info: <path d="M9 18h6m-5 3h4M12 3a6 6 0 0 0-3.5 10.9c.6.4.9 1 .9 1.6V16h5.2v-.5c0-.6.3-1.2.9-1.6A6 6 0 0 0 12 3Z" />,
  success: <path d="M12 21a9 9 0 1 0 0-18 9 9 0 0 0 0 18Zm-4-9 3 3 5-6" />,
}

// Lists keep the order returned by the backend; `limit` shows only the first few.
function FindingList({ title, items, tone, ordered = false, limit, icon = false }) {
  const list = toList(items)
  const shown = limit ? list.slice(0, limit) : list
  const ListTag = ordered ? 'ol' : 'ul'
  return (
    <div className={cn('finding-group', `finding-${tone}`)}>
      <h3 className="finding-title">
        {icon ? (
          <span className="finding-icon" aria-hidden="true">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">{FINDING_ICONS[tone]}</svg>
          </span>
        ) : null}
        {title} <span className="finding-count">{list.length}<span className="sr-only"> total</span></span>
      </h3>
      {shown.length ? (
        <ListTag className="finding-list">{shown.map((item, index) => <li key={`${index}-${item}`} style={{ '--i': index }}>{item}</li>)}</ListTag>
      ) : <p className="panel-note">None reported.</p>}
      {list.length > shown.length ? <p className="finding-more">+{list.length - shown.length} more</p> : null}
    </div>
  )
}

function prefersReducedMotion() {
  return typeof window !== 'undefined' && Boolean(window.matchMedia?.('(prefers-reduced-motion: reduce)').matches)
}

// Counts up to `value` on mount; screen readers only get the final number.
function CountUp({ value, duration = 900 }) {
  const [shown, setShown] = useState(() => (prefersReducedMotion() ? value : 0))
  useEffect(() => {
    if (prefersReducedMotion()) {
      setShown(value)
      return undefined
    }
    let frame
    const start = performance.now()
    const step = (now) => {
      const t = Math.min((now - start) / duration, 1)
      setShown(Math.round(value * (1 - (1 - t) ** 3)))
      if (t < 1) frame = requestAnimationFrame(step)
    }
    frame = requestAnimationFrame(step)
    // Animation frames pause in hidden tabs; make sure the final value still lands.
    const settle = window.setTimeout(() => setShown(value), duration + 250)
    return () => {
      cancelAnimationFrame(frame)
      window.clearTimeout(settle)
    }
  }, [value, duration])
  return <><span aria-hidden="true">{shown.toLocaleString()}</span><span className="sr-only">{value.toLocaleString()}</span></>
}

function KpiGrid({ items, label }) {
  const visible = items.filter(([, value]) => value !== null && value !== undefined && value !== '')
  if (!visible.length) return null
  return (
    <dl className="kpi-grid" aria-label={label}>
      {visible.map(([name, value, detail, icon], index) => (
        <div className="kpi-card" key={name} style={{ '--i': index }}>
          <dt>{icon ? <span className="kpi-icon" aria-hidden="true"><LayerIcon name={icon} size={14} /></span> : null}{name}</dt>
          <dd>{typeof value === 'number' ? <CountUp value={value} /> : value}{detail ? <> <span className="kpi-detail">{detail}</span></> : null}</dd>
        </div>
      ))}
    </dl>
  )
}

function numberOrNull(value) {
  return typeof value === 'number' && Number.isFinite(value) ? value.toLocaleString() : null
}

function finiteOrNull(value) {
  return typeof value === 'number' && Number.isFinite(value) ? value : null
}

function CardTitle({ id, icon, children }) {
  return (
    <h2 id={id} className="card-title">
      <span className="card-icon" aria-hidden="true"><LayerIcon name={icon} size={16} /></span>
      {children}
    </h2>
  )
}

function RepositoryStrip({ analysis, submittedUrl }) {
  const meta = analysis.metadata || null
  const identity = resolveRepositoryIdentity(meta, submittedUrl)
  const facts = meta ? [
    ['Stars', numberOrNull(meta.stars)],
    ['Forks', numberOrNull(meta.forks)],
    ['Watchers', numberOrNull(meta.watchers)],
    ['Open issues', numberOrNull(meta.openIssues)],
    ['License', meta.license],
    ['Visibility', meta.visibility],
    ['Default branch', meta.defaultBranch],
    ['Last push', meta.pushedAt ? formatDate(meta.pushedAt) : null],
    ['Topics', toList(meta.topics).join(', ') || null],
  ].filter(([, value]) => value) : []
  return (
    <section className="repo-strip" aria-labelledby="overview-repository">
      <h2 id="overview-repository" className="card-title">
        <span className="card-icon" aria-hidden="true"><LayerIcon name="globe" size={16} /></span>
        Repository
      </h2>
      {identity.link ? <a className="text-link repo-link" href={identity.link} target="_blank" rel="noopener noreferrer">{identity.link}</a> : null}
      {meta?.description ? <p className="repo-description">{meta.description}</p> : null}
      {facts.length ? (
        <dl className="fact-list">
          {facts.map(([label, value], index) => <div key={label} style={{ '--i': index }}><dt>{label}</dt><dd>{value}</dd></div>)}
        </dl>
      ) : <p className="panel-note">GitHub metadata is not available for this analysis.</p>}
    </section>
  )
}

function OverviewHealthCard({ health, onNavigate }) {
  const weakest = buildCategoryRows(health?.categories).filter((row) => row.ratio < 1).slice(0, 3)
  const hasScore = typeof health?.overallScore === 'number'
  return (
    <article className="dashboard-card card-health" aria-labelledby="overview-health">
      <header className="card-header">
        <CardTitle id="overview-health" icon="shield">Health</CardTitle>
        <TabLink tab="health" onNavigate={onNavigate}>View health details</TabLink>
      </header>
      {hasScore ? (
        <div className="health-summary">
          <ScoreGauge score={health.overallScore} grade={health.grade} />
          <div className="health-summary-body">
            <GradeBadge grade={health.grade} />
            {weakest.length ? (
              <>
                <h3 className="micro-label">Lowest-scoring categories</h3>
                <ul className="mini-scores">
                  {weakest.map((row) => (
                    <li key={row.key}>
                      <span className="mini-score-label">{row.label}</span>
                      <Meter value={row.score} max={row.maxScore} size="sm" />
                      <span className="mini-score-value">{row.score}/{row.maxScore}<span className="sr-only"> points</span></span>
                    </li>
                  ))}
                </ul>
              </>
            ) : null}
          </div>
        </div>
      ) : <p className="panel-note">Health scoring is not available for this analysis.</p>}
    </article>
  )
}

function OverviewFileTypesCard({ languages, onNavigate }) {
  const { rows, total } = useMemo(() => buildExtensionRows(languages), [languages])
  return (
    <article className="dashboard-card card-files" aria-labelledby="overview-file-types">
      <header className="card-header">
        <CardTitle id="overview-file-types" icon="chart">File types</CardTitle>
        <TabLink tab="languages" onNavigate={onNavigate}>View all file types</TabLink>
      </header>
      {rows.length ? (
        <>
          <DistributionBar segments={topSegments(rows, total, OVERVIEW_EXTENSION_LIMIT)} caption="Share of files by extension" />
          <p className="panel-note">{fileLabel(total)} grouped by extension. Counts files, not lines.</p>
        </>
      ) : <p className="panel-note">No file extension data available.</p>}
    </article>
  )
}

function OverviewTechnologyCard({ tech, onNavigate }) {
  const names = tech ? uniqueTechnologyNames(buildTechGroups(tech)) : []
  return (
    <article className="dashboard-card dashboard-card-wide card-tech" aria-labelledby="overview-technology">
      <header className="card-header">
        <CardTitle id="overview-technology" icon="box">Technology</CardTitle>
        <TabLink tab="technology" onNavigate={onNavigate}>View technology details</TabLink>
      </header>
      {tech ? (
        <>
          {tech.repositoryType || typeof tech.confidenceScore === 'number' ? (
            <p className="tech-headline">
              {tech.repositoryType ? <strong>{tech.repositoryType}</strong> : null}
              {typeof tech.confidenceScore === 'number' ? <span>{tech.confidenceScore}% confidence</span> : null}
            </p>
          ) : null}
          {names.length ? (
            <ul className="chip-list" aria-label="Detected technologies">
              {names.map((name, index) => <li className="chip" key={name} style={{ '--i': index }}>{name}</li>)}
            </ul>
          ) : <p className="panel-note">No technology markers were detected.</p>}
        </>
      ) : <p className="panel-note">Technology detection is not available for this analysis.</p>}
    </article>
  )
}

// Prefers the scored health findings; falls back to insight lists when health is missing.
function OverviewFindingsCard({ health, insights, onNavigate }) {
  const source = health
    ? { tab: 'health', groups: [['Warnings', health.warnings, 'warning', false], ['Recommendations', health.recommendations, 'info', true], ['Strengths', health.strengths, 'success', false]] }
    : insights
      ? { tab: 'insights', groups: [['Risks', insights.risks, 'warning', false], ['Suggestions', insights.suggestions, 'info', true], ['Strengths', insights.strengths, 'success', false]] }
      : null
  return (
    <article className="dashboard-card dashboard-card-wide card-findings" aria-labelledby="overview-findings">
      <header className="card-header">
        <CardTitle id="overview-findings" icon="alert">Key findings</CardTitle>
        {source ? <TabLink tab={source.tab} onNavigate={onNavigate}>View all findings</TabLink> : null}
      </header>
      {source ? (
        <div className="finding-stack">
          {source.groups.map(([title, items, tone, ordered]) => (
            <FindingList key={title} title={title} items={items} tone={tone} ordered={ordered} limit={OVERVIEW_FINDING_LIMIT} icon />
          ))}
        </div>
      ) : <p className="panel-note">No findings are available for this analysis.</p>}
    </article>
  )
}

function OverviewTab({ analysis, submittedUrl, onNavigate }) {
  const health = analysis.repositoryHealth
  const languages = analysis.languages
  const kpis = [
    ['Files', finiteOrNull(analysis.fileCount), null, 'file'],
    ['Directories', finiteOrNull(analysis.dirCount), null, 'folder'],
    ['Total lines', finiteOrNull(analysis.totalLines), 'All files', 'code'],
    ['File types', languages && typeof languages === 'object' ? Object.keys(languages).length : null, null, 'layers'],
    ['Health score', finiteOrNull(health?.overallScore), '/ 100', 'gauge'],
    ['Complexity', analysis.repositoryInsights?.estimatedComplexity || null, null, 'workers'],
  ]
  return (
    <div className="overview">
      <KpiGrid items={kpis} label="Key metrics" />
      <div className="dashboard-grid">
        <OverviewHealthCard health={health} onNavigate={onNavigate} />
        <OverviewFileTypesCard languages={languages} onNavigate={onNavigate} />
        <OverviewTechnologyCard tech={analysis.technologyAnalysis} onNavigate={onNavigate} />
        <OverviewFindingsCard health={health} insights={analysis.repositoryInsights} onNavigate={onNavigate} />
      </div>
      <RepositoryStrip analysis={analysis} submittedUrl={submittedUrl} />
    </div>
  )
}

function TechnologyStack({ tech }) {
  if (!tech) return null
  const groups = buildTechGroups(tech)
  const detected = groups.filter((group) => group.items.length > 0)
  const notDetected = groups.filter((group) => group.reportedEmpty && group.key !== 'frameworks')
  const hasConfidence = typeof tech.confidenceScore === 'number'
  return (
    <section className="report-section" aria-labelledby="technology-heading">
      <ReportHeading id="technology-heading" title="Technology stack" description="Detected from repository files. Evidence shows the file or signal behind each detection." />
      <dl className="tech-summary">
        <div>
          <dt>Repository type</dt>
          <dd>{tech.repositoryType || 'Unknown'}</dd>
        </div>
        {hasConfidence ? (
          <div>
            <dt>Overall confidence</dt>
            <dd className="meter-row"><Meter value={tech.confidenceScore} />{tech.confidenceScore}%</dd>
          </div>
        ) : null}
      </dl>
      {detected.length ? (
        <div className="tech-grid">
          {detected.map((group) => (
            <article className="tech-card" key={group.key} aria-labelledby={`tech-${group.key}`}>
              <h3 id={`tech-${group.key}`}>{group.label}</h3>
              <ul className="tech-items">
                {group.items.map((item) => (
                  <li key={item.name}>
                    <div className="tech-item-head">
                      <strong>{item.name}</strong>
                      {typeof item.confidence === 'number' ? (
                        <span className="meter-row tech-item-confidence">
                          <Meter value={item.confidence} size="sm" />
                          {item.confidence}%<span className="sr-only"> confidence</span>
                        </span>
                      ) : null}
                    </div>
                    {item.reason ? <p className="tech-evidence"><span className="sr-only">Evidence: </span>{item.reason}</p> : null}
                  </li>
                ))}
              </ul>
            </article>
          ))}
        </div>
      ) : <p className="panel-note">No technology markers were detected for this repository.</p>}
      {notDetected.length ? (
        <p className="panel-note"><strong>Not detected:</strong> {notDetected.map((group) => group.label).join(', ')}</p>
      ) : null}
    </section>
  )
}

function FileTypes({ languages }) {
  const { rows, total } = useMemo(() => buildExtensionRows(languages), [languages])
  const description = rows.length
    ? `Share of ${fileLabel(total)} by extension. Counts files, not lines of code.`
    : 'Counts files by extension, not lines of code.'
  return (
    <section className="report-section" aria-labelledby="languages-heading">
      <ReportHeading id="languages-heading" title="File types" description={description} />
      {rows.length ? (
        <>
          <DistributionBar segments={topSegments(rows, total, FILE_TYPES_EXTENSION_LIMIT)} caption="Largest file types" showValues={false} />
          <div className="report-subsection">
            <h3>All extensions</h3>
            <ul className="bar-list plain-list file-type-list">
              {rows.map((row) => (
                <li className="bar-row" key={row.key}>
                  <span className={cn('bar-label', row.key === '(none)' ? '' : 'mono')}>{row.label}</span>
                  <Meter value={row.percent} />
                  <span className="bar-value">{fileLabel(row.count)} · {formatPercent(row.percent, row.percent < 1 ? 1 : 0)}</span>
                </li>
              ))}
            </ul>
          </div>
        </>
      ) : <p className="panel-note">No file extension data available.</p>}
    </section>
  )
}

function DocumentationChecklist({ documentation }) {
  const entries = Object.entries(documentation || {}).filter(([, value]) => typeof value === 'boolean')
  if (!entries.length) return null
  return (
    <div className="report-subsection">
      <h3>Documentation files</h3>
      <ul className="check-list">
        {entries.map(([key, present]) => (
          <li key={key} className={present ? 'is-present' : 'is-missing'}>
            <span className="check-mark" aria-hidden="true">{present ? '✓' : '✕'}</span>
            <span>{DOCUMENTATION_LABELS[key] || humanizeKey(key)}</span>
            <span className="check-status">{present ? 'Found' : 'Missing'}</span>
          </li>
        ))}
      </ul>
    </div>
  )
}

function RepositoryHealth({ health, documentation }) {
  if (!health) return null
  const categories = buildCategoryRows(health.categories)
  return (
    <section className="report-section" aria-labelledby="health-heading">
      <ReportHeading id="health-heading" title="Repository health" description="Scores come from static repository signals only." />
      <div className="health-layout">
        <div className="health-score">
          {typeof health.overallScore === 'number' ? <ScoreGauge score={health.overallScore} grade={health.grade} size="lg" /> : <p className="panel-note">Overall score unavailable.</p>}
          <GradeBadge grade={health.grade} />
        </div>
        <div className="report-subsection">
          <h3>Category scores <span className="heading-note">lowest first</span></h3>
          {categories.length ? (
            <ul className="score-list">
              {categories.map((row) => (
                <li className="score-row" key={row.key}>
                  <span className="score-label">{row.label}</span>
                  <Meter value={row.score} max={row.maxScore} />
                  <span className="score-points">{row.score}/{row.maxScore}<span className="sr-only"> points</span></span>
                  <GradeBadge grade={row.grade} compact />
                </li>
              ))}
            </ul>
          ) : <p className="panel-note">No category scores were reported.</p>}
        </div>
      </div>
      <div className="finding-grid">
        <FindingList title="Warnings" items={health.warnings} tone="warning" />
        <FindingList title="Recommendations" items={health.recommendations} tone="info" ordered />
        <FindingList title="Strengths" items={health.strengths} tone="success" />
      </div>
      <DocumentationChecklist documentation={documentation} />
    </section>
  )
}

function EngineeringInsights({ insights }) {
  if (!insights) return null
  const narrative = [['Summary', insights.summary], ['Technology', insights.technologyOverview], ['Quality', insights.qualityOverview]].filter(([, text]) => text)
  const assessments = [
    ['Complexity', insights.estimatedComplexity || null],
    ['Project size', insights.estimatedProjectSize || null],
    ['Maturity', insights.estimatedMaturity || null],
  ]
  return (
    <section className="report-section report-insights" aria-labelledby="insights-heading">
      <ReportHeading id="insights-heading" title="Engineering insights" description="Rule-based assessments generated from the analysis result." />
      <KpiGrid items={assessments} label="Assessments" />
      <div className="finding-grid">
        <FindingList title="Risks" items={insights.risks} tone="warning" icon />
        <FindingList title="Suggestions" items={insights.suggestions} tone="info" ordered icon />
        <FindingList title="Strengths" items={insights.strengths} tone="success" icon />
      </div>
      {narrative.length ? (
        <details className="report-details">
          <summary>Generated summary</summary>
          <div className="insight-text">
            {narrative.map(([title, text]) => <div key={title}><h3>{title}</h3><p>{text}</p></div>)}
          </div>
        </details>
      ) : null}
    </section>
  )
}

export function AnalysisReport({ analysis, submittedUrl }) {
  const tabs = REPORT_TABS.filter(([, , isAvailable]) => isAvailable(analysis))
  const [selectedTab, setSelectedTab] = useState('overview')
  const activeTab = tabs.some(([key]) => key === selectedTab) ? selectedTab : 'overview'

  const selectTab = (key) => {
    setSelectedTab(key)
    document.getElementById(`report-tab-${key}`)?.focus()
  }

  const navigation = {
    canNavigate: (key) => tabs.some(([tabKey]) => tabKey === key),
    go: selectTab,
  }

  const handleTabKeyDown = (event) => {
    const index = tabs.findIndex(([key]) => key === activeTab)
    const keyToIndex = { ArrowRight: (index + 1) % tabs.length, ArrowLeft: (index - 1 + tabs.length) % tabs.length, Home: 0, End: tabs.length - 1 }
    if (!(event.key in keyToIndex)) return
    event.preventDefault()
    selectTab(tabs[keyToIndex[event.key]][0])
  }

  return (
    <section className="report" aria-label="Analysis report">
      <div className="report-tabs" role="tablist" aria-label="Analysis report sections" onKeyDown={handleTabKeyDown}>
        {tabs.map(([key, label]) => (
          <button
            key={key}
            id={`report-tab-${key}`}
            type="button"
            role="tab"
            className={cn('report-tab', key === activeTab ? 'is-active' : '')}
            aria-selected={key === activeTab}
            aria-controls="report-panel"
            tabIndex={key === activeTab ? 0 : -1}
            onClick={() => setSelectedTab(key)}
          >
            {label}
          </button>
        ))}
      </div>
      <div id="report-panel" role="tabpanel" aria-labelledby={`report-tab-${activeTab}`} className="report-panel" tabIndex={0}>
        {activeTab === 'overview' ? <OverviewTab analysis={analysis} submittedUrl={submittedUrl} onNavigate={navigation} /> : null}
        {activeTab === 'health' ? <RepositoryHealth health={analysis.repositoryHealth} documentation={analysis.technologyAnalysis?.documentation} /> : null}
        {activeTab === 'technology' ? <TechnologyStack tech={analysis.technologyAnalysis} /> : null}
        {activeTab === 'languages' ? <FileTypes languages={analysis.languages} /> : null}
        {activeTab === 'insights' ? <EngineeringInsights insights={analysis.repositoryInsights} /> : null}
      </div>
    </section>
  )
}
