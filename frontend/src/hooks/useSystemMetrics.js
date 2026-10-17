import { useCallback, useEffect, useState } from 'react'
import { fetchMetricsText, fetchSystemSummary } from '../lib/api.js'
import { parsePrometheusMetrics } from '../lib/metrics.js'

const REFRESH_INTERVAL_MS = 15000

export function useSystemMetrics() {
  const [metricsState, setMetricsState] = useState({ data: null, error: '', lastUpdated: '' })
  const [systemState, setSystemState] = useState({ data: null, error: '', lastUpdated: '' })

  const refreshMetrics = useCallback(async () => {
    try {
      const text = await fetchMetricsText()
      const parsed = parsePrometheusMetrics(text)
      setMetricsState({ data: parsed, error: '', lastUpdated: new Date().toISOString() })
    } catch (error) {
      setMetricsState((current) => ({ data: current.data, error: `Metrics unavailable: ${error.message}`, lastUpdated: current.lastUpdated }))
    }
  }, [])

  const refreshSystemSummary = useCallback(async () => {
    try {
      const payload = await fetchSystemSummary()
      if (!payload.success) {
        throw new Error(payload.message || 'System summary unavailable')
      }
      setSystemState({ data: payload.data, error: '', lastUpdated: new Date().toISOString() })
    } catch (error) {
      setSystemState((current) => ({ data: current.data, error: `System summary unavailable: ${error.message}`, lastUpdated: current.lastUpdated }))
    }
  }, [])

  useEffect(() => {
    refreshMetrics()
    refreshSystemSummary()
    const timer = window.setInterval(() => {
      refreshMetrics()
      refreshSystemSummary()
    }, REFRESH_INTERVAL_MS)
    return () => window.clearInterval(timer)
  }, [refreshMetrics, refreshSystemSummary])

  return { metricsState, systemState, refreshMetrics, refreshSystemSummary }
}
