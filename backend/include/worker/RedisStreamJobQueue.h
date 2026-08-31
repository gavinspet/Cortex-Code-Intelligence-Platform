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

namespace drogon::nosql {
class RedisResult;
}

namespace cortex::worker {

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
