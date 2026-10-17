const getApiUrl = () => {
  const envUrl = import.meta.env.VITE_API_URL
  if (envUrl) return envUrl
  if (typeof window !== 'undefined' && window.location.hostname.includes('vercel.app')) {
    return 'https://cortex-code-intelligence-platform.onrender.com'
  }
  return 'http://localhost:8080'
}

export const API = getApiUrl()

const jobPath = (jobId) => encodeURIComponent(jobId)

export async function submitRepository(repositoryUrl) {
  const response = await fetch(`${API}/repositories`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ repositoryUrl }),
  })
  return response.json()
}

export async function fetchJob(jobId) {
  const response = await fetch(`${API}/jobs/${jobPath(jobId)}`)
  return response.json()
}

export async function fetchJobDetails(jobId) {
  const response = await fetch(`${API}/jobs/${jobPath(jobId)}/details`)
  return response.json()
}

export async function fetchAnalysis(jobId) {
  const response = await fetch(`${API}/analysis/${jobPath(jobId)}`)
  return response.json()
}

export async function fetchMetricsText() {
  const response = await fetch(`${API}/metrics`)
  return response.text()
}

export async function fetchSystemSummary() {
  const response = await fetch(`${API}/system/summary`)
  return response.json()
}
