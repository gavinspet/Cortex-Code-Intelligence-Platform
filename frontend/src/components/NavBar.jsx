import { useState } from 'react'
import { cn } from './ui.jsx'
import { CortexMark } from './BrandIcons.jsx'

const NAV_ITEMS = [
  ['home', '#/', 'Home'],
  ['architecture', '#/architecture', 'Technical Architecture'],
  ['about', '#/about', 'About'],
  ['contact', '#/contact', 'Contact'],
  ['system', '#/system', 'System'],
]

// The analysis report is reached from Home, so Home stays highlighted there.
const activeKeyFor = (routeName) => (routeName === 'analysis' ? 'home' : routeName)

export function NavBar({ routeName }) {
  const [menuOpen, setMenuOpen] = useState(false)
  const activeKey = activeKeyFor(routeName)

  return (
    <header className="site-header">
      <nav className="site-nav" aria-label="Main">
        <a className="site-brand" href="#/"><CortexMark size={30} />Cortex</a>
        <button
          type="button"
          className="site-nav-toggle"
          aria-expanded={menuOpen}
          aria-controls="site-nav-links"
          onClick={() => setMenuOpen((open) => !open)}
        >
          Menu
        </button>
        <ul id="site-nav-links" className={cn('site-nav-links', menuOpen ? 'is-open' : '')}>
          {NAV_ITEMS.map(([key, href, label]) => (
            <li key={key}>
              <a
                href={href}
                className={cn('site-nav-link', key === activeKey ? 'is-active' : '')}
                aria-current={key === activeKey ? 'page' : undefined}
                onClick={() => setMenuOpen(false)}
              >
                {label}
              </a>
            </li>
          ))}
        </ul>
      </nav>
    </header>
  )
}
