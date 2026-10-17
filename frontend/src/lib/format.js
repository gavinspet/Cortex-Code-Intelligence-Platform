const SUPPORTED_REPO_HOST_PATTERNS = ['github.com', 'gitlab.com', 'bitbucket.org', 'dev.azure.com']

export function validateRepositoryUrl(rawUrl) {
  const value = (rawUrl || '').trim()
  if (!value) return { isValid: false, message: 'Enter a repository URL to continue.' }

  let url
  try {
    url = new URL(value)
  } catch {
    return { isValid: false, message: 'Use a valid URL format, for example https://github.com/user/repo.' }
  }

  if (url.protocol !== 'https:' && url.protocol !== 'http:') {
    return { isValid: false, message: 'Repository URLs must start with http:// or https://.' }
  }

  const host = url.hostname.toLowerCase()
  const supported = SUPPORTED_REPO_HOST_PATTERNS.some((pattern) => host === pattern || host.endsWith(`.${pattern}`))
  if (!supported) {
    return { isValid: false, message: 'Supported hosts: GitHub, GitLab, Bitbucket, and Azure DevOps.' }
  }

  const parts = url.pathname.replace(/\.git$/, '').split('/').filter(Boolean)
  if (parts.length < 2) {
    return { isValid: false, message: 'Repository URLs must include both owner or group and repository name.' }
  }

  return { isValid: true, message: '' }
}

export function parseRepoFromUrl(rawUrl) {
  if (!rawUrl) return null
  try {
    const url = new URL(rawUrl)
    const parts = url.pathname.replace(/\.git$/, '').split('/').filter(Boolean)
    if (parts.length >= 2) {
      return {
        owner: parts[0],
        name: parts[1],
        fullName: `${parts[0]}/${parts[1]}`,
        githubLink: `${url.protocol}//${url.host}/${parts[0]}/${parts[1]}`,
      }
    }
  } catch {
    return null
  }
  return null
}

export function toLocale(value) {
  return typeof value === 'number' ? value.toLocaleString() : value || '0'
}

export function formatNumber(value, fallback = 'Unavailable') {
  return typeof value === 'number' && Number.isFinite(value) ? value.toLocaleString() : fallback
}

export function formatPercent(value, digits = 0) {
  if (typeof value !== 'number' || Number.isNaN(value)) return 'Unavailable'
  return `${value.toFixed(digits)}%`
}

export function formatDate(value) {
  if (!value) return 'Unavailable'
  const date = new Date(value)
  return Number.isNaN(date.getTime()) ? 'Unavailable' : date.toLocaleString()
}

export function durationBetween(startValue, endValue) {
  if (!startValue || !endValue) return null
  const start = new Date(startValue).getTime()
  const end = new Date(endValue).getTime()
  if (Number.isNaN(start) || Number.isNaN(end) || end < start) return null
  return end - start
}

export function formatDuration(durationMs) {
  if (typeof durationMs !== 'number' || Number.isNaN(durationMs) || durationMs < 0) return 'Unavailable'
  if (durationMs < 1000) return `${Math.round(durationMs)} ms`
  const seconds = durationMs / 1000
  if (seconds < 60) return `${seconds.toFixed(seconds < 10 ? 1 : 0)} s`
  const minutes = Math.floor(seconds / 60)
  return `${minutes}m ${Math.round(seconds % 60)}s`
}
