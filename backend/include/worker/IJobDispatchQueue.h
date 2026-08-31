/**
 * @file IJobDispatchQueue.h
 * @brief Abstraction for dispatching jobs through a message queue.
 */

#pragma once

#include <optional>
#include <string>

namespace cortex::worker {

struct StreamJobMessage {
    std::string streamId;
    std::string jobId;
};

class IJobDispatchQueue {
public:
    virtual ~IJobDispatchQueue() = default;

    virtual bool ensureConsumerGroup() noexcept = 0;
    virtual bool publishJob(const std::string& jobId) noexcept = 0;
    virtual std::optional<StreamJobMessage> consumeNext(const std::string& consumerName,
                                                        int blockMs) noexcept = 0;
    virtual bool ack(const std::string& streamId) noexcept = 0;

protected:
    IJobDispatchQueue() = default;
    IJobDispatchQueue(const IJobDispatchQueue&) = delete;
    IJobDispatchQueue& operator=(const IJobDispatchQueue&) = delete;
};

} // namespace cortex::worker
