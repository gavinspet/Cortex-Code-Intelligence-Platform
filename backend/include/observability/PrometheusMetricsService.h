#pragma once

#include "observability/IMetrics.h"
#include <array>
#include <atomic>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace cortex::observability {

class PrometheusMetricsService final : public IMetrics {
public:
    PrometheusMetricsService() noexcept;

    void incrementJobsSubmitted() noexcept override;
    void incrementJobsCompleted() noexcept override;
    void incrementJobsFailed() noexcept override;
    void incrementJobsRetried() noexcept override;
    void incrementJobsDeadLettered() noexcept override;

    void incrementJobsActive() noexcept override;
    void decrementJobsActive() noexcept override;
    void setJobsQueueDepth(double depth) noexcept override;

    void observeJobProcessingDurationSeconds(double seconds) noexcept override;

    void observeHttpRequest(std::string_view method,
                            std::string_view route,
                            int statusCode,
                            double durationSeconds) noexcept override;

    std::string renderPrometheus() const noexcept override;

private:
    static constexpr std::array<double, 11> kDurationBuckets{
        0.01, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0, 30.0, 60.0
    };

    struct HttpHistogramState {
        std::vector<unsigned long long> buckets;
        unsigned long long count{0};
        double sum{0.0};
    };

    static std::string escapeLabelValue(std::string_view value);
    static std::string makeHttpLabelKey(std::string_view method,
                                        std::string_view route,
                                        int statusCode);

    std::atomic<unsigned long long> jobsSubmitted_{0};
    std::atomic<unsigned long long> jobsCompleted_{0};
    std::atomic<unsigned long long> jobsFailed_{0};
    std::atomic<unsigned long long> jobsRetried_{0};
    std::atomic<unsigned long long> jobsDeadLettered_{0};

    std::atomic<long long> jobsActive_{0};
    std::atomic<double> jobsQueueDepth_{0.0};

    std::array<std::atomic<unsigned long long>, kDurationBuckets.size()> jobDurationBucketCounts_{};
    std::atomic<unsigned long long> jobDurationCount_{0};
    std::atomic<double> jobDurationSum_{0.0};

    mutable std::mutex httpMutex_;
    std::unordered_map<std::string, unsigned long long> httpRequestCounts_;
    std::unordered_map<std::string, HttpHistogramState> httpDurationHistograms_;
};

} // namespace cortex::observability
