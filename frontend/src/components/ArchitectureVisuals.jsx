import { cn } from './ui.jsx'

// Diagram building blocks for the Technical Architecture page. Colour always
// reinforces a visible text label; it never carries meaning on its own.

export const LAYERS = {
  frontend: 'Frontend',
  api: 'API',
  queue: 'Queue & workers',
  analysis: 'Analysis',
  data: 'Persistence',
  external: 'External',
  observability: 'Observability',
}

export const TAGS = {
  production: { label: 'Production', meaning: 'Confirmed on the live deployment' },
  configurable: { label: 'Configurable', meaning: 'Chosen by environment variables' },
  'best-effort': { label: 'Best-effort', meaning: 'A failure here does not fail the job' },
  volatile: { label: 'Lost on restart', meaning: 'Held in process memory only' },
  local: { label: 'Local Compose only', meaning: 'Started by docker-compose.yml' },
  unverified: { label: 'Not configured in repo', meaning: 'Depends on deployment settings' },
}

// Approximate brand colours, used only as a small swatch beside the written name.
const TECH_COLORS = {
  React: '#61dafb',
  Vite: '#646cff',
  Vercel: '#111827',
  'C++': '#00599c',
  Drogon: '#64748b',
  Docker: '#2496ed',
  Redis: '#dc382d',
  MySQL: '#00758f',
  Git: '#f05032',
  GitHub: '#24292f',
  OpenTelemetry: '#f5a800',
  Prometheus: '#e6522c',
  Grafana: '#f46800',
  Jaeger: '#60d0e4',
}

const ICONS = {
  browser: <><rect x="3" y="4" width="18" height="16" rx="2" /><path d="M3 9h18M6.5 6.5h.01M9 6.5h.01" /></>,
  cloud: <path d="M7 18h10a4 4 0 0 0 .6-7.95A6 6 0 0 0 6.1 11 3.5 3.5 0 0 0 7 18z" />,
  server: <><rect x="4" y="4" width="16" height="6" rx="1.5" /><rect x="4" y="14" width="16" height="6" rx="1.5" /><path d="M8 7h.01M8 17h.01" /></>,
  queue: <><rect x="3" y="7" width="4" height="10" rx="1" /><rect x="10" y="7" width="4" height="10" rx="1" /><rect x="17" y="7" width="4" height="10" rx="1" /></>,
  workers: <><rect x="7" y="7" width="10" height="10" rx="1.5" /><path d="M10 3v4M14 3v4M10 17v4M14 17v4M3 10h4M3 14h4M17 10h4M17 14h4" /></>,
  analysis: <><circle cx="11" cy="11" r="6" /><path d="m16 16 4 4" /></>,
  database: <><ellipse cx="12" cy="6" rx="7" ry="3" /><path d="M5 6v12c0 1.66 3.13 3 7 3s7-1.34 7-3V6" /><path d="M5 12c0 1.66 3.13 3 7 3s7-1.34 7-3" /></>,
  memory: <><rect x="3" y="7" width="18" height="10" rx="1.5" /><path d="M7 10v4M11 10v4M15 10v4M7 17v3M12 17v3M17 17v3" /></>,
  globe: <><circle cx="12" cy="12" r="9" /><path d="M3 12h18M12 3c2.5 2.7 3.8 5.7 3.8 9s-1.3 6.3-3.8 9c-2.5-2.7-3.8-5.7-3.8-9S9.5 5.7 12 3z" /></>,
  chart: <><path d="M4 4v16h16" /><path d="m7 15 4-4 3 3 5-6" /></>,
  trace: <><circle cx="6" cy="6" r="2" /><circle cx="18" cy="12" r="2" /><circle cx="6" cy="18" r="2" /><path d="M8 6h4a4 4 0 0 1 4 4M8 18h4a4 4 0 0 0 4-4" /></>,
  alert: <><path d="M12 3 2 20h20L12 3z" /><path d="M12 10v4M12 17h.01" /></>,
  retry: <><path d="M20 11a8 8 0 1 0-2.3 5.7" /><path d="M20 4v7h-7" /></>,
  gauge: <><path d="M4 17a8 8 0 1 1 16 0" /><path d="m12 17 4-5" /></>,
  inbox: <><path d="M3 13h5l1.5 3h5l1.5-3h5" /><path d="M5.5 5h13L21 13v6H3v-6l2.5-8z" /></>,
  shield: <path d="M12 3 4 6v6c0 4.5 3.4 8.2 8 9 4.6-.8 8-4.5 8-9V6l-8-3z" />,
  layers: <><path d="m12 3 9 5-9 5-9-5 9-5z" /><path d="m3 13 9 5 9-5" /></>,
  route: <><circle cx="6" cy="18" r="2" /><circle cx="18" cy="6" r="2" /><path d="M8 18h6a3 3 0 0 0 0-6h-4a3 3 0 0 1 0-6h6" /></>,
  check: <><circle cx="12" cy="12" r="9" /><path d="m8 12 3 3 5-6" /></>,
  box: <><path d="m12 3 8 4.5v9L12 21l-8-4.5v-9L12 3z" /><path d="m4 7.5 8 4.5 8-4.5M12 12v9" /></>,
  file: <><path d="M14 3H7a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h10a2 2 0 0 0 2-2V8l-5-5z" /><path d="M14 3v5h5" /></>,
  folder: <path d="M3 7a2 2 0 0 1 2-2h4l2 2h8a2 2 0 0 1 2 2v8a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V7z" />,
  code: <path d="m9 8-4 4 4 4M15 8l4 4-4 4" />,
}

export function LayerIcon({ name, size = 18 }) {
  return (
    <svg
      className="arch-icon"
      viewBox="0 0 24 24"
      width={size}
      height={size}
      fill="none"
      stroke="currentColor"
      strokeWidth="1.8"
      strokeLinecap="round"
      strokeLinejoin="round"
      aria-hidden="true"
      focusable="false"
    >
      {ICONS[name]}
    </svg>
  )
}

export function Arrow({ className, both = false }) {
  return (
    <span className={cn('arch-arrow', className)} aria-hidden="true">
      <svg viewBox="0 0 24 12" width="24" height="12" fill="none" stroke="currentColor" strokeWidth="1.8" strokeLinecap="round" strokeLinejoin="round" focusable="false">
        <path d="M2 6h20M17 1.5 22 6l-5 4.5" />
        {both ? <path d="M7 1.5 2 6l5 4.5" /> : null}
      </svg>
    </span>
  )
}

export function TechMarks({ names }) {
  if (!names?.length) return null
  return (
    <ul className="arch-tech-list">
      {names.map((name) => (
        <li key={name} className="arch-tech">
          <span className="arch-tech-swatch" style={{ '--mark': TECH_COLORS[name] || '#64748b' }} aria-hidden="true" />
          {name}
        </li>
      ))}
    </ul>
  )
}

export function EnvTag({ kind }) {
  const tag = TAGS[kind]
  if (!tag) return null
  return <span className={cn('arch-tag', `arch-tag-${kind}`)}>{tag.label}</span>
}

export function IconTile({ layer, icon, size }) {
  return <span className={cn('arch-icon-tile', `layer-${layer}`)}><LayerIcon name={icon} size={size} /></span>
}

export function SectionTitle({ id, layer, icon, title, children }) {
  return (
    <header className="arch-section-head">
      <h2 id={id}>
        <IconTile layer={layer} icon={icon} />
        {title}
      </h2>
      {children ? <p>{children}</p> : null}
    </header>
  )
}

export function Legend() {
  return (
    <div className="arch-legend">
      <div className="arch-legend-group">
        <p className="arch-legend-title" id="arch-legend-layers">Layer colours</p>
        <ul className="arch-legend-list" aria-labelledby="arch-legend-layers">
          {Object.entries(LAYERS).map(([key, label]) => (
            <li key={key} className={cn('arch-legend-item', `layer-${key}`)}>
              <span className="arch-swatch" aria-hidden="true" />
              {label}
            </li>
          ))}
        </ul>
      </div>
      <div className="arch-legend-group">
        <p className="arch-legend-title" id="arch-legend-tags">Labels</p>
        <ul className="arch-legend-list" aria-labelledby="arch-legend-tags">
          {Object.keys(TAGS).map((kind) => (
            <li key={kind} className="arch-legend-item">
              <EnvTag kind={kind} />
              <span className="arch-legend-meaning">{TAGS[kind].meaning}</span>
            </li>
          ))}
        </ul>
      </div>
    </div>
  )
}

export function ArchNode({ node, element: Element = 'div', showLayer = false, compact = false, stacked = false }) {
  const { layer, icon, title, tech, note, code, tag } = node
  return (
    <Element className={cn('arch-node', `layer-${layer}`, compact && 'arch-node-compact', stacked && 'arch-node-stacked', tag === 'unverified' && 'is-dashed')}>
      {stacked ? (
        <>
          <div className="arch-node-head">
            <IconTile layer={layer} icon={icon} />
            <span className="arch-node-layer">{LAYERS[layer]}</span>
          </div>
          <span className="arch-node-title">{title}</span>
          <span className="arch-node-watermark" aria-hidden="true"><LayerIcon name={icon} size={88} /></span>
        </>
      ) : (
        <div className="arch-node-head">
          <IconTile layer={layer} icon={icon} />
          <div className="arch-node-heading">
            {showLayer ? <span className="arch-node-layer">{LAYERS[layer]}</span> : null}
            <span className="arch-node-title">{title}</span>
          </div>
        </div>
      )}
      {tag ? <EnvTag kind={tag} /> : null}
      <TechMarks names={tech} />
      {note ? <p className="arch-node-note">{note}</p> : null}
      {code ? <code className="arch-code">{code}</code> : null}
    </Element>
  )
}

// Main end-to-end diagram: numbered stages, each holding one or two components.
export function StageDiagram({ label, stages }) {
  return (
    <ol className="arch-stages" aria-label={label}>
      {stages.map((stage, index) => (
        <li key={stage.title} className={cn('arch-stage', stage.nodes.length === 1 && 'is-solo')} style={{ '--i': index }}>
          <p className="arch-stage-head">
            <span className="arch-stage-number" aria-hidden="true">{index + 1}</span>
            <span><span className="sr-only">Stage {index + 1}: </span>{stage.title}</span>
          </p>
          <ul className="arch-stage-nodes">
            {stage.nodes.map((node) => <ArchNode key={node.title} node={node} element="li" stacked />)}
          </ul>
          {index < stages.length - 1 ? <Arrow className="arch-stage-arrow" /> : null}
        </li>
      ))}
    </ol>
  )
}

function LinkList({ label, links }) {
  return (
    <ul className="arch-links" aria-label={label}>
      {links.map((link) => (
        <li key={`${link.from}-${link.to}`} className="arch-link">
          <span className="arch-link-route">{link.from} <span aria-hidden="true">↔</span><span className="sr-only">to</span> {link.to}</span>
          <Arrow both className="arch-link-arrow" />
          <span className="arch-link-via">{link.via}</span>
        </li>
      ))}
    </ul>
  )
}

// Where each component runs and how the pieces connect.
export function TopologyDiagram({ clients, clientLinks, container, serviceLinks, services }) {
  return (
    <div className="arch-topology">
      <div className="arch-topology-col">
        <p className="arch-topology-title">Clients</p>
        <ul className="arch-topology-nodes">
          {clients.map((node) => <ArchNode key={node.title} node={node} element="li" compact />)}
        </ul>
      </div>
      <LinkList label="Client connections" links={clientLinks} />
      <section className="arch-container layer-api" aria-labelledby="arch-container-title">
        <div className="arch-container-head">
          <IconTile layer="api" icon="box" />
          <div>
            <h3 id="arch-container-title">{container.title}</h3>
            <p>{container.note}</p>
          </div>
        </div>
        <TechMarks names={container.tech} />
        <ul className="arch-container-nodes">
          {container.nodes.map((node) => <ArchNode key={node.title} node={node} element="li" compact />)}
        </ul>
      </section>
      <LinkList label="Backing service connections" links={serviceLinks} />
      <div className="arch-topology-col">
        <p className="arch-topology-title">Backing services</p>
        <ul className="arch-topology-nodes">
          {services.map((node) => <ArchNode key={node.title} node={node} element="li" compact />)}
        </ul>
      </div>
    </div>
  )
}

export function StepList({ steps }) {
  return (
    <ol className="arch-steps">
      {steps.map((step, index) => (
        <li key={step.title} className={cn('arch-step', `layer-${step.layer}`)}>
          <div className="arch-step-head">
            <span className="arch-step-number" aria-hidden="true">{index + 1}</span>
            <div>
              <span className="arch-step-layer">{LAYERS[step.layer]}</span>
              <h3 className="arch-step-title"><span className="sr-only">Step {index + 1}: </span>{step.title}</h3>
            </div>
          </div>
          <p className="arch-step-text">{step.text}</p>
          {step.code ? <code className="arch-code">{step.code}</code> : null}
        </li>
      ))}
    </ol>
  )
}

// Compact horizontal pipeline; becomes vertical on narrow screens.
export function Chain({ label, items }) {
  return (
    <ol className="arch-chain" aria-label={label}>
      {items.map((item, index) => (
        <li key={item.label} className="arch-chain-item">
          <div className={cn('arch-chain-node', `layer-${item.layer}`, item.dashed && 'is-dashed')}>
            <span className="arch-chain-label">{item.label}</span>
            {item.detail ? <span className="arch-chain-detail">{item.detail}</span> : null}
            {item.tag ? <EnvTag kind={item.tag} /> : null}
          </div>
          {index < items.length - 1 ? <Arrow className="arch-chain-arrow" /> : null}
        </li>
      ))}
    </ol>
  )
}

export function InfoCard({ layer, icon, title, tag, children, element: Element = 'div' }) {
  return (
    <Element className={cn('arch-card', `layer-${layer}`)}>
      <IconTile layer={layer} icon={icon} />
      <div className="arch-card-body">
        <div className="arch-card-head">
          <h3 className="arch-card-title">{title}</h3>
          {tag ? <EnvTag kind={tag} /> : null}
        </div>
        {children}
      </div>
    </Element>
  )
}

export function ChipList({ label, items, mono = false, muted = false }) {
  return (
    <ul className={cn('arch-chips', mono && 'is-mono', muted && 'is-muted')} aria-label={label}>
      {items.map((item) => <li key={item}>{item}</li>)}
    </ul>
  )
}
