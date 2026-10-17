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

function ThemeIcon({ dark }) {
  return (
    <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" aria-hidden="true">
      {dark ? (
        <path d="M21 12.8A9 9 0 1 1 11.2 3a7 7 0 0 0 9.8 9.8z" />
      ) : (
        <>
          <circle cx="12" cy="12" r="4" />
          <path d="M12 2v2M12 20v2M4.9 4.9l1.4 1.4M17.7 17.7l1.4 1.4M2 12h2M20 12h2M4.9 19.1l1.4-1.4M17.7 6.3l1.4-1.4" />
        </>
      )}
    </svg>
  )
}

export function NavBar({ routeName, theme, onToggleTheme }) {
  const [menuOpen, setMenuOpen] = useState(false)
  const activeKey = activeKeyFor(routeName)
  const isDark = theme === 'dark'

  return (
    <header className="site-header">
      <nav className="site-nav" aria-label="Main">
        <a className="site-brand" href="#/"><CortexMark size={30} />Cortex</a>
        <div className="site-nav-actions">
          <button
            type="button"
            className="theme-toggle"
            role="switch"
            aria-checked={isDark}
            aria-label="Dark mode"
            title={isDark ? 'Switch to light mode' : 'Switch to dark mode'}
            onClick={onToggleTheme}
          >
            <span key={theme} className="theme-toggle-icon"><ThemeIcon dark={!isDark} /></span>
          </button>
          <button
            type="button"
            className="site-nav-toggle"
            aria-expanded={menuOpen}
            aria-controls="site-nav-links"
            onClick={() => setMenuOpen((open) => !open)}
          >
            Menu
          </button>
        </div>
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
