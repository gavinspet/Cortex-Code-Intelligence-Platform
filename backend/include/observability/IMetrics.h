#pragma once

#include <string>
#include <string_view>

namespace cortex::observability {

class IMetrics {
public:
    virtual ~IMetrics() = default;

    virtual void incrementJobsSubmitted() noexcept = 0;
    virtual void incrementJobsCompleted() noexcept = 0;
    virtual void incrementJobsFailed() noexcept = 0;
    virtual void incrementJobsRetried() noexcept = 0;
    virtual void incrementJobsDeadLettered() noexcept = 0;

    virtual void incrementJobsActive() noexcept = 0;
    virtual void decrementJobsActive() noexcept = 0;
    virtual void setJobsQueueDepth(double depth) noexcept = 0;

    virtual void observeJobProcessingDurationSeconds(double seconds) noexcept = 0;

    virtual void observeHttpRequest(std::string_view method,
                                    std::string_view route,
                                    int statusCode,
                                    double durationSeconds) noexcept = 0;

    virtual std::string renderPrometheus() const noexcept = 0;
};

} // namespace cortex::observability
