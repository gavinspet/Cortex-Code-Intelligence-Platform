/**
 * @file RedisStreamJobQueue.h
 * @brief Redis Streams implementation of IJobDispatchQueue.
 */

#pragma once

#include "worker/IJobDispatchQueue.h"
#include <atomic>
#include <functional>
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
                        std::string groupName) noexcept;

    bool ensureConsumerGroup() noexcept override;
    bool publishJob(const std::string& jobId) noexcept override;
    std::optional<StreamJobMessage> consumeNext(const std::string& consumerName,
                                                int blockMs) noexcept override;
    bool ack(const std::string& streamId) noexcept override;

private:
    std::optional<StreamJobMessage> parseStreamMessage(const drogon::nosql::RedisResult& result) const;

    std::string redisClientName_;
    std::string streamName_;
    std::string groupName_;

    std::atomic<bool> groupEnsured_{false};
    std::mutex ensureMutex_;
};

} // namespace cortex::worker
