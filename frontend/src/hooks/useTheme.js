import { useCallback, useEffect, useState } from 'react'

// index.html applies the saved theme before the first paint; this hook keeps it in sync afterwards.
const STORAGE_KEY = 'cortex-theme'
export const THEMES = ['light', 'dark']

function readTheme() {
  const current = document.documentElement.dataset.theme
  return THEMES.includes(current) ? current : 'light'
}

export function useTheme() {
  const [theme, setThemeState] = useState(readTheme)

  useEffect(() => {
    document.documentElement.dataset.theme = theme
    try {
      localStorage.setItem(STORAGE_KEY, theme)
    } catch {
      // Storage can be unavailable (private mode); the theme still applies for this visit.
    }
  }, [theme])

  const setTheme = useCallback((next) => {
    if (!THEMES.includes(next)) return
    const apply = () => { document.documentElement.dataset.theme = next }
    const reduceMotion = window.matchMedia?.('(prefers-reduced-motion: reduce)').matches
    // Cross-fade between themes where the browser supports view transitions.
    // A skipped transition (e.g. hidden tab) still applies the theme, so its rejection is ignored.
    if (document.startViewTransition && !reduceMotion) document.startViewTransition(apply).ready.catch(() => {})
    else apply()
    setThemeState(next)
  }, [])

  return [theme, setTheme]
}
