/**
 * @file IJobDispatchQueue.h
 * @brief Abstraction for dispatching jobs through a message queue.
 */

#pragma once

#include "observability/ITracing.h"

#include <optional>
#include <string>

namespace cortex::worker {

struct StreamJobMessage {
    std::string streamId;
    std::string jobId;
    int attempt{0};
    std::string traceparent;
    std::string tracestate;
};

class IJobDispatchQueue {
public:
    virtual ~IJobDispatchQueue() = default;

    virtual bool ensureConsumerGroup() noexcept = 0;
    virtual bool publishJob(const std::string& jobId,
                            int attempt = 0,
                            const std::optional<cortex::observability::TraceContext>& traceContext = std::nullopt) noexcept = 0;
    virtual std::optional<StreamJobMessage> consumeNext(const std::string& consumerName,
                                                        int blockMs) noexcept = 0;
    virtual bool ack(const std::string& streamId) noexcept = 0;
    virtual bool publishDeadLetter(const StreamJobMessage& message,
                                   const std::string& reason,
                                   const std::optional<cortex::observability::TraceContext>& traceContext = std::nullopt) noexcept = 0;

protected:
    IJobDispatchQueue() = default;
    IJobDispatchQueue(const IJobDispatchQueue&) = delete;
    IJobDispatchQueue& operator=(const IJobDispatchQueue&) = delete;
};

} // namespace cortex::worker
