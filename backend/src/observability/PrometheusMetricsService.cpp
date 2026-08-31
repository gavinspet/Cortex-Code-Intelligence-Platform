#include "observability/PrometheusMetricsService.h"
#include <algorithm>
#include <charconv>
#include <sstream>

namespace cortex::observability {

PrometheusMetricsService::PrometheusMetricsService() noexcept {
    for (auto& item : jobDurationBucketCounts_) {
        item.store(0);
    }
}

void PrometheusMetricsService::incrementJobsSubmitted() noexcept {
    jobsSubmitted_.fetch_add(1, std::memory_order_relaxed);
}

void PrometheusMetricsService::incrementJobsCompleted() noexcept {
    jobsCompleted_.fetch_add(1, std::memory_order_relaxed);
}

void PrometheusMetricsService::incrementJobsFailed() noexcept {
    jobsFailed_.fetch_add(1, std::memory_order_relaxed);
}

void PrometheusMetricsService::incrementJobsRetried() noexcept {
    jobsRetried_.fetch_add(1, std::memory_order_relaxed);
}

void PrometheusMetricsService::incrementJobsDeadLettered() noexcept {
    jobsDeadLettered_.fetch_add(1, std::memory_order_relaxed);
}

void PrometheusMetricsService::incrementJobsActive() noexcept {
    jobsActive_.fetch_add(1, std::memory_order_relaxed);
}

void PrometheusMetricsService::decrementJobsActive() noexcept {
    const long long current = jobsActive_.load(std::memory_order_relaxed);
    if (current <= 0) {
        jobsActive_.store(0, std::memory_order_relaxed);
        return;
    }
    jobsActive_.fetch_sub(1, std::memory_order_relaxed);
}

void PrometheusMetricsService::setJobsQueueDepth(double depth) noexcept {
    jobsQueueDepth_.store(std::max(0.0, depth), std::memory_order_relaxed);
}

void PrometheusMetricsService::observeJobProcessingDurationSeconds(double seconds) noexcept {
    if (seconds < 0.0) {
        return;
    }

    for (size_t i = 0; i < kDurationBuckets.size(); ++i) {
        if (seconds <= kDurationBuckets[i]) {
            jobDurationBucketCounts_[i].fetch_add(1, std::memory_order_relaxed);
        }
    }

    jobDurationCount_.fetch_add(1, std::memory_order_relaxed);

    double current = jobDurationSum_.load(std::memory_order_relaxed);
    while (!jobDurationSum_.compare_exchange_weak(
        current,
        current + seconds,
        std::memory_order_relaxed,
        std::memory_order_relaxed)) {
    }
}

void PrometheusMetricsService::observeHttpRequest(std::string_view method,
                                                  std::string_view route,
                                                  int statusCode,
                                                  double durationSeconds) noexcept {
    if (durationSeconds < 0.0) {
        return;
    }

    const std::string key = makeHttpLabelKey(method, route, statusCode);

    std::lock_guard<std::mutex> lock(httpMutex_);
    httpRequestCounts_[key] += 1;

    auto& hist = httpDurationHistograms_[key];
    if (hist.buckets.empty()) {
        hist.buckets.resize(kDurationBuckets.size(), 0);
    }

    for (size_t i = 0; i < kDurationBuckets.size(); ++i) {
        if (durationSeconds <= kDurationBuckets[i]) {
            hist.buckets[i] += 1;
        }
    }

    hist.count += 1;
    hist.sum += durationSeconds;
}

std::string PrometheusMetricsService::renderPrometheus() const noexcept {
    try {
        std::ostringstream out;
        out.setf(std::ios::fixed);
        out.precision(6);

        out << "# HELP cortex_jobs_submitted_total Total number of jobs submitted\n";
        out << "# TYPE cortex_jobs_submitted_total counter\n";
        out << "cortex_jobs_submitted_total " << jobsSubmitted_.load(std::memory_order_relaxed) << "\n";

        out << "# HELP cortex_jobs_completed_total Total number of jobs completed successfully\n";
        out << "# TYPE cortex_jobs_completed_total counter\n";
        out << "cortex_jobs_completed_total " << jobsCompleted_.load(std::memory_order_relaxed) << "\n";

        out << "# HELP cortex_jobs_failed_total Total number of jobs that permanently failed\n";
        out << "# TYPE cortex_jobs_failed_total counter\n";
        out << "cortex_jobs_failed_total " << jobsFailed_.load(std::memory_order_relaxed) << "\n";

        out << "# HELP cortex_jobs_retried_total Total number of scheduled job retries\n";
        out << "# TYPE cortex_jobs_retried_total counter\n";
        out << "cortex_jobs_retried_total " << jobsRetried_.load(std::memory_order_relaxed) << "\n";

        out << "# HELP cortex_jobs_dead_lettered_total Total number of jobs moved to dead-letter stream\n";
        out << "# TYPE cortex_jobs_dead_lettered_total counter\n";
        out << "cortex_jobs_dead_lettered_total " << jobsDeadLettered_.load(std::memory_order_relaxed) << "\n";

        out << "# HELP cortex_jobs_active Current number of jobs actively being processed\n";
        out << "# TYPE cortex_jobs_active gauge\n";
        out << "cortex_jobs_active " << jobsActive_.load(std::memory_order_relaxed) << "\n";

        out << "# HELP cortex_jobs_queue_depth Current Redis stream load used by backpressure\n";
        out << "# TYPE cortex_jobs_queue_depth gauge\n";
        out << "cortex_jobs_queue_depth " << jobsQueueDepth_.load(std::memory_order_relaxed) << "\n";

        out << "# HELP cortex_job_processing_duration_seconds Job processing duration in seconds\n";
        out << "# TYPE cortex_job_processing_duration_seconds histogram\n";
        for (size_t i = 0; i < kDurationBuckets.size(); ++i) {
            out << "cortex_job_processing_duration_seconds_bucket{le=\""
                << kDurationBuckets[i]
                << "\"} "
                << jobDurationBucketCounts_[i].load(std::memory_order_relaxed)
                << "\n";
        }
        out << "cortex_job_processing_duration_seconds_bucket{le=\"+Inf\"} "
            << jobDurationCount_.load(std::memory_order_relaxed) << "\n";
        out << "cortex_job_processing_duration_seconds_sum "
            << jobDurationSum_.load(std::memory_order_relaxed) << "\n";
        out << "cortex_job_processing_duration_seconds_count "
            << jobDurationCount_.load(std::memory_order_relaxed) << "\n";

        out << "# HELP cortex_http_requests_total Total HTTP requests\n";
        out << "# TYPE cortex_http_requests_total counter\n";

        out << "# HELP cortex_http_request_duration_seconds HTTP request latency in seconds\n";
        out << "# TYPE cortex_http_request_duration_seconds histogram\n";

        std::lock_guard<std::mutex> lock(httpMutex_);
        for (const auto& [key, count] : httpRequestCounts_) {
            out << "cortex_http_requests_total{" << key << "} " << count << "\n";
        }

        for (const auto& [key, hist] : httpDurationHistograms_) {
            for (size_t i = 0; i < kDurationBuckets.size(); ++i) {
                out << "cortex_http_request_duration_seconds_bucket{" << key
                    << ",le=\"" << kDurationBuckets[i] << "\"} " << hist.buckets[i] << "\n";
            }
            out << "cortex_http_request_duration_seconds_bucket{" << key << ",le=\"+Inf\"} "
                << hist.count << "\n";
            out << "cortex_http_request_duration_seconds_sum{" << key << "} " << hist.sum << "\n";
            out << "cortex_http_request_duration_seconds_count{" << key << "} " << hist.count << "\n";
        }

        return out.str();
    } catch (...) {
        return "# HELP cortex_metrics_export_errors_total Number of metric export errors\n"
               "# TYPE cortex_metrics_export_errors_total counter\n"
               "cortex_metrics_export_errors_total 1\n";
    }
}

std::string PrometheusMetricsService::escapeLabelValue(std::string_view value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (char c : value) {
        if (c == '\\' || c == '"') {
            escaped.push_back('\\');
        }
        escaped.push_back(c);
    }
    return escaped;
}

std::string PrometheusMetricsService::makeHttpLabelKey(std::string_view method,
                                                       std::string_view route,
                                                       int statusCode) {
    char statusBuf[16]{};
    const auto [ptr, ec] = std::to_chars(statusBuf, statusBuf + sizeof(statusBuf), statusCode);
    std::string status = (ec == std::errc()) ? std::string(statusBuf, ptr) : "500";

    return std::string("method=\"") + escapeLabelValue(method) +
           "\",route=\"" + escapeLabelValue(route) +
           "\",status=\"" + status + "\"";
}

} // namespace cortex::observability
