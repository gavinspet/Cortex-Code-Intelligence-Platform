import { cn, PageHeader } from '../components/ui.jsx'
import { BrandLogo, CortexMark } from '../components/BrandIcons.jsx'
import { LayerIcon } from '../components/ArchitectureVisuals.jsx'

const REPORT_ITEMS = [
  { icon: 'layers', label: 'File, directory and line counts' },
  { icon: 'chart', label: 'File types' },
  { icon: 'box', label: 'Detected technologies' },
  { icon: 'gauge', label: 'Rule-based health score' },
  { icon: 'check', label: 'Strengths, risks and suggestions' },
  { icon: 'globe', label: 'GitHub metadata, when available' },
]

const BY_HAND = ['Read through directories', 'Open build files one by one', 'Check the CI configuration']
const WITH_CORTEX = ['One automated first pass', "See the repository's shape", 'Spot obvious gaps before reading code']

const AUDIENCE = [
  { layer: 'frontend', icon: 'analysis', title: 'Evaluators', text: 'Developers evaluating an open-source project.' },
  { layer: 'queue', icon: 'route', title: 'Newcomers', text: 'Engineers getting started on an unfamiliar repository.' },
  { layer: 'data', icon: 'check', title: 'Reviewers', text: 'Reviewers who want a quick, consistent first look at a codebase.' },
]

function WhatItIs() {
  return (
    <section className="about-block" aria-labelledby="about-what">
      <h2 id="about-what">What it is</h2>
      <ol className="about-flow" aria-label="From repository URL to report">
        <li className="about-flow-card">
          <span className="about-flow-logos">
            <BrandLogo id="github" />
            <BrandLogo id="gitlab" />
          </span>
          <strong>Repository URL</strong>
          <span>A public GitHub or GitLab repository</span>
          <span className="about-flow-link" aria-hidden="true"><span /></span>
        </li>
        <li className="about-flow-card about-flow-core">
          <CortexMark size={44} className="about-flow-mark" />
          <strong>Cortex</strong>
          <span>Clones it and inspects the files</span>
          <span className="about-flow-link" aria-hidden="true"><span /></span>
        </li>
        <li className="about-flow-card about-flow-report">
          <strong>Structured report</strong>
          <ul className="about-report-items">
            {REPORT_ITEMS.map((item, index) => (
              <li key={item.label} style={{ '--i': index }}>
                <LayerIcon name={item.icon} size={16} />
                {item.label}
              </li>
            ))}
          </ul>
        </li>
      </ol>
    </section>
  )
}

function Problem() {
  return (
    <section className="about-block" aria-labelledby="about-problem">
      <h2 id="about-problem">The problem it addresses</h2>
      <div className="about-compare">
        <div className="about-compare-col is-before">
          <p className="about-compare-title"><LayerIcon name="alert" size={18} />By hand</p>
          <ul>{BY_HAND.map((item) => <li key={item}>{item}</li>)}</ul>
        </div>
        <span className="about-compare-arrow" aria-hidden="true">→</span>
        <div className="about-compare-col is-after">
          <p className="about-compare-title"><LayerIcon name="check" size={18} />With Cortex</p>
          <ul>{WITH_CORTEX.map((item) => <li key={item}>{item}</li>)}</ul>
        </div>
      </div>
      <p className="about-note">
        <LayerIcon name="shield" size={16} />
        Static inspection only — Cortex does not build or run the project.
      </p>
    </section>
  )
}

function Audience() {
  return (
    <section className="about-block" aria-labelledby="about-who">
      <h2 id="about-who">Who it is for</h2>
      <ul className="about-audience">
        {AUDIENCE.map((person) => (
          <li key={person.title} className={cn('about-person', `layer-${person.layer}`)}>
            <span className="about-person-icon"><LayerIcon name={person.icon} size={22} /></span>
            <h3>{person.title}</h3>
            <p>{person.text}</p>
          </li>
        ))}
      </ul>
    </section>
  )
}

export function AboutPage() {
  return (
    <div className="about">
      <PageHeader
        eyebrow="About"
        title="About Cortex"
        description="A repository analysis service for a fast, consistent first look at any public codebase."
      />
      <WhatItIs />
      <Problem />
      <Audience />
      <div className="about-cta">
        <a className="button-primary" href="#/">Analyse a repository</a>
        <a className="button-secondary" href="#/architecture">Read how Cortex works</a>
      </div>
    </div>
  )
}
