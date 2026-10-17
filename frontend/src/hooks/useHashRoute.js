import { useEffect, useState } from 'react'

const STATIC_PAGES = ['architecture', 'about', 'contact', 'system', 'appearance']
const JOB_ID_PATTERN = /^[A-Za-z0-9-]{1,64}$/

export function parseHash(hash) {
  const segments = (hash || '').replace(/^#/, '').split('/').filter(Boolean)
  if (segments.length === 0) return { name: 'home' }
  const [first, second] = segments
  if (segments.length === 1 && STATIC_PAGES.includes(first)) return { name: first }
  if (segments.length === 2 && first === 'analysis' && JOB_ID_PATTERN.test(second)) {
    return { name: 'analysis', jobId: second }
  }
  return { name: 'not-found' }
}

export function navigate(path) {
  window.location.hash = path
}

export function useHashRoute() {
  const [route, setRoute] = useState(() => parseHash(window.location.hash))

  useEffect(() => {
    const onHashChange = () => {
      setRoute(parseHash(window.location.hash))
      window.scrollTo(0, 0)
    }
    window.addEventListener('hashchange', onHashChange)
    return () => window.removeEventListener('hashchange', onHashChange)
  }, [])

  return route
}
