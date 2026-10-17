import { useCallback, useEffect, useMemo, useState } from 'react'
import { validateRepositoryUrl } from './lib/format.js'
import { PROFILE_LINKS, PROJECT_LINKS } from './lib/links.js'
import { useAnalysisJob } from './hooks/useAnalysisJob.js'
import { navigate, parseHash, useHashRoute } from './hooks/useHashRoute.js'
import { NavBar } from './components/NavBar.jsx'
import { PageHeader } from './components/ui.jsx'
import { BrandLogo, CortexMark } from './components/BrandIcons.jsx'
import { EXAMPLE_REPOSITORY, HomePage } from './pages/Home.jsx'
import { AnalysisPage } from './pages/Analysis.jsx'
import { ArchitecturePage } from './pages/Architecture.jsx'
import { AboutPage } from './pages/About.jsx'
import { ContactPage } from './pages/Contact.jsx'
import { SystemPage } from './pages/System.jsx'

const FOOTER_SOCIALS = [
  ['github', 'GitHub', PROFILE_LINKS.github],
  ['linkedin', 'LinkedIn', PROFILE_LINKS.linkedin],
  ['instagram', 'Instagram', PROFILE_LINKS.instagram],
  ['email', 'Email', PROFILE_LINKS.email],
]

function Footer() {
  return (
    <footer className="footer-shell">
      <div className="footer-brand">
        <CortexMark size={32} />
        <div><p className="footer-title">Cortex</p><p className="footer-copy">Distributed code intelligence for public repositories.</p></div>
      </div>
      <div className="footer-links"><a href={PROJECT_LINKS.repo} target="_blank" rel="noopener noreferrer">Source</a><a href={PROJECT_LINKS.health} target="_blank" rel="noopener noreferrer">Health</a><a href={PROJECT_LINKS.metrics} target="_blank" rel="noopener noreferrer">Metrics</a></div>
      <ul className="footer-socials">
        {FOOTER_SOCIALS.map(([id, label, href]) => (
          <li key={id}>
            <a
              className="footer-social"
              href={href}
              aria-label={id === 'email' ? label : `${label} (opens in a new tab)`}
              title={label}
              {...(id === 'email' ? {} : { target: '_blank', rel: 'noopener noreferrer' })}
            >
              <BrandLogo id={id} />
            </a>
          </li>
        ))}
      </ul>
    </footer>
  )
}

function NotFoundPage() {
  return (
    <PageHeader title="Page not found" description="The page you requested does not exist.">
      <p><a className="text-link" href="#/">Go to the home page</a></p>
    </PageHeader>
  )
}

export default function App() {
  const route = useHashRoute()
  const [url, setUrl] = useState(EXAMPLE_REPOSITORY)
  const urlValidation = useMemo(() => validateRepositoryUrl(url), [url])
  const analysisJob = useAnalysisJob()
  const { job, phase, submit, load } = analysisJob

  // Opening #/analysis/{jobId} directly (refresh, bookmark, shared link) starts tracking that job.
  const routeJobId = route.name === 'analysis' ? route.jobId : null
  useEffect(() => {
    if (routeJobId && routeJobId !== job?.jobId && phase !== 'submitting') load(routeJobId)
  }, [routeJobId, job?.jobId, phase, load])

  const handleSubmit = useCallback(async (event) => {
    event.preventDefault()
    const jobId = await submit(url)
    // Only redirect if the user is still on Home when the job is accepted.
    if (jobId && parseHash(window.location.hash).name === 'home') navigate(`/analysis/${jobId}`)
  }, [submit, url])

  let page
  switch (route.name) {
    case 'home':
      page = <HomePage url={url} setUrl={setUrl} validation={urlValidation} onSubmit={handleSubmit} analysisJob={analysisJob} />
      break
    case 'analysis':
      page = <AnalysisPage jobId={route.jobId} analysisJob={analysisJob} />
      break
    case 'architecture':
      page = <ArchitecturePage />
      break
    case 'about':
      page = <AboutPage />
      break
    case 'contact':
      page = <ContactPage />
      break
    case 'system':
      page = <SystemPage analysisJob={analysisJob} />
      break
    default:
      page = <NotFoundPage />
  }

  return (
    <div className="app-shell">
      <NavBar routeName={route.name} />
      <main className="page-shell" role="main">
        {page}
      </main>
      <Footer />
    </div>
  )
}
