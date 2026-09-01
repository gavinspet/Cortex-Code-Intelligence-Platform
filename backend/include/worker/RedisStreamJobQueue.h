/**
 * @file RedisStreamJobQueue.h
 * @brief Redis Streams implementation of IJobDispatchQueue.
 */

#pragma once

#include "worker/IJobDispatchQueue.h"
#include "observability/IMetrics.h"
#include "observability/ITracing.h"
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace drogon::nosql {
class RedisResult;
}

namespace cortex::worker {

struct QueueConsumerSnapshot {
    std::string name;
    long long pending{0};
    long long idleMs{0};
};

struct QueueGroupSnapshot {
    std::string stream;
    std::string consumerGroup;
    long long pending{0};
    long long lag{0};
    std::vector<QueueConsumerSnapshot> consumers;
};

struct JobDispatchSnapshot {
    int attempt{0};
    bool hasDispatchRecord{false};
    bool pending{false};
    bool deadLettered{false};
    long long deliveryCount{0};
    std::string streamId;
    std::string consumerName;
    std::string failureReason;
    std::string traceparent;
    std::string tracestate;
};

class RedisStreamJobQueue : public IJobDispatchQueue {
public:
    RedisStreamJobQueue(std::string redisClientName,
                        std::string streamName,
                        std::string groupName,
                        std::shared_ptr<cortex::observability::IMetrics> metrics = nullptr,
                        std::shared_ptr<cortex::observability::ITracing> tracing = nullptr) noexcept;

    bool ensureConsumerGroup() noexcept override;
    bool publishJob(const std::string& jobId,
                    int attempt = 0,
                    const std::optional<cortex::observability::TraceContext>& traceContext = std::nullopt) noexcept override;
    std::optional<StreamJobMessage> consumeNext(const std::string& consumerName,
                                                int blockMs) noexcept override;
    bool ack(const std::string& streamId) noexcept override;
    bool publishDeadLetter(const StreamJobMessage& message,
                           const std::string& reason,
                           const std::optional<cortex::observability::TraceContext>& traceContext = std::nullopt) noexcept override;

    std::optional<QueueGroupSnapshot> getQueueGroupSnapshot() noexcept;
    std::optional<JobDispatchSnapshot> getJobDispatchSnapshot(const std::string& jobId) noexcept;

    const std::string& streamName() const noexcept { return streamName_; }
    const std::string& groupName() const noexcept { return groupName_; }

private:
    std::optional<StreamJobMessage> parseStreamMessage(const drogon::nosql::RedisResult& result) const;
    bool isBackpressured() noexcept;
    long long resolveMaxBacklog() const noexcept;

    std::string redisClientName_;
    std::string streamName_;
    std::string groupName_;
    std::string deadLetterStreamName_;
    std::shared_ptr<cortex::observability::IMetrics> metrics_;
    std::shared_ptr<cortex::observability::ITracing> tracing_;

    std::atomic<bool> groupEnsured_{false};
    std::mutex ensureMutex_;
    std::mutex commandMutex_;
};

} // namespace cortex::worker
