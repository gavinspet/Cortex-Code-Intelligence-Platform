import { useCallback, useEffect, useRef, useState } from 'react'
import { fetchAnalysis, fetchJob, fetchJobDetails, submitRepository } from '../lib/api.js'
import { validateRepositoryUrl } from '../lib/format.js'

const POLL_INTERVAL_MS = 2000

/**
 * Owns the repository-analysis lifecycle: submit -> poll job -> fetch analysis.
 * Only one job is tracked at a time. `activeJobIdRef` identifies it; responses
 * that arrive for any other job (e.g. after the user opened a different
 * analysis link) are ignored so they cannot overwrite the current state.
 */
export function useAnalysisJob() {
  const [submittedUrl, setSubmittedUrl] = useState('')
  const [phase, setPhase] = useState('idle')
  const [statusMessage, setStatusMessage] = useState('')
  const [errorMessage, setErrorMessage] = useState('')
  const [analysis, setAnalysis] = useState(null)
  const [job, setJob] = useState(null)
  const [jobDetails, setJobDetails] = useState(null)
  const pollTimeout = useRef(null)
  const pollGeneration = useRef(0)
  const activeJobIdRef = useRef(null)

  const isActive = (jobId) => activeJobIdRef.current === jobId

  // Bumping the generation also invalidates any poll request already in flight,
  // so restarting polling (e.g. load() called twice) cannot leave a second loop running.
  const stopPolling = useCallback(() => {
    pollGeneration.current += 1
    if (pollTimeout.current) window.clearTimeout(pollTimeout.current)
    pollTimeout.current = null
  }, [])

  const refreshJobDetails = useCallback(async (jobId) => {
    if (!jobId) return
    try {
      const payload = await fetchJobDetails(jobId)
      if (!isActive(jobId)) return
      if (payload.success && payload.data) {
        setJobDetails(payload.data)
      }
    } catch {
      // Keep last known job details rather than replacing them with synthetic placeholders.
    }
  }, [])

  useEffect(() => stopPolling, [stopPolling])

  const finishWithAnalysis = useCallback(async (jobId) => {
    const payload = await fetchAnalysis(jobId)
    if (!isActive(jobId)) return
    if (!payload.success) {
      setPhase('error')
      setErrorMessage(payload.message || 'Analysis was not available after the job completed.')
      setStatusMessage('The job completed, but the analysis payload could not be loaded.')
      return
    }
    setAnalysis(payload.data)
    setPhase('done')
    setErrorMessage('')
    setStatusMessage('Analysis complete. The full dashboard is now rendering live backend data.')
    refreshJobDetails(jobId)
  }, [refreshJobDetails])

  const pollJob = useCallback(async (jobId) => {
    stopPolling()
    const generation = pollGeneration.current
    const isCurrent = () => generation === pollGeneration.current && isActive(jobId)
    const tick = async () => {
      if (!isCurrent()) return
      try {
        const payload = await fetchJob(jobId)
        if (!isCurrent()) return
        if (!payload.success || !payload.data) {
          setPhase('error')
          setErrorMessage(payload.message || 'Job state could not be loaded.')
          setStatusMessage('The frontend lost visibility into the active job.')
          return
        }
        setJob(payload.data)
        refreshJobDetails(jobId)
        const status = (payload.data.status || '').toUpperCase()
        if (status === 'COMPLETED') {
          await finishWithAnalysis(jobId)
          return
        }
        if (status === 'FAILED') {
          setPhase('error')
          setErrorMessage('The backend marked the analysis job as FAILED.')
          setStatusMessage('The worker pipeline ended the job in a failed state.')
          return
        }
        setPhase('polling')
        setErrorMessage('')
        setStatusMessage(`Live job state: ${status || 'UNKNOWN'}`)
        pollTimeout.current = window.setTimeout(tick, POLL_INTERVAL_MS)
      } catch (error) {
        if (!isCurrent()) return
        setPhase('error')
        setErrorMessage(`Job polling failed: ${error.message}`)
        setStatusMessage('The browser could not continue polling the live job endpoint.')
      }
    }
    await tick()
  }, [finishWithAnalysis, refreshJobDetails, stopPolling])

  /**
   * Submits a repository and starts polling in the background.
   * Resolves to the new job ID once the backend accepts it, or null otherwise.
   */
  const submit = useCallback(async (rawUrl) => {
    const cleanUrl = (rawUrl || '').trim()
    const validation = validateRepositoryUrl(cleanUrl)
    if (!validation.isValid) {
      setPhase('error')
      setErrorMessage(validation.message)
      setStatusMessage('Validation failed before the job was submitted.')
      return null
    }
    stopPolling()
    activeJobIdRef.current = null
    setSubmittedUrl(cleanUrl)
    setPhase('submitting')
    setStatusMessage('Submitting repository for analysis...')
    setErrorMessage('')
    setAnalysis(null)
    setJob(null)
    setJobDetails(null)
    try {
      const payload = await submitRepository(cleanUrl)
      if (!payload.success || !payload.data?.jobId) {
        setPhase('error')
        setErrorMessage(payload.message || 'The backend rejected the repository submission.')
        setStatusMessage('Submission was not accepted by the distributed job pipeline.')
        return null
      }
      const jobId = payload.data.jobId
      activeJobIdRef.current = jobId
      setJob({ jobId, repositoryUrl: cleanUrl, status: payload.data.status || 'QUEUED', createdAt: payload.data.createdAt || new Date().toISOString(), startedAt: payload.data.startedAt, completedAt: payload.data.completedAt })
      setStatusMessage(`Job accepted. Job ID: ${jobId}`)
      refreshJobDetails(jobId)
      pollJob(jobId)
      return jobId
    } catch (error) {
      setPhase('error')
      setErrorMessage(`Request failed: ${error.message}`)
      setStatusMessage('The browser could not reach the analysis API.')
      return null
    }
  }, [pollJob, refreshJobDetails, stopPolling])

  /** Starts tracking an existing job, e.g. when an analysis link is opened directly. */
  const load = useCallback((jobId) => {
    stopPolling()
    activeJobIdRef.current = jobId
    setSubmittedUrl('')
    setPhase('polling')
    setStatusMessage('Loading job status...')
    setErrorMessage('')
    setAnalysis(null)
    setJob({ jobId })
    setJobDetails(null)
    refreshJobDetails(jobId)
    pollJob(jobId)
  }, [pollJob, refreshJobDetails, stopPolling])

  return { submittedUrl, phase, statusMessage, errorMessage, analysis, job, jobDetails, submit, load }
}
