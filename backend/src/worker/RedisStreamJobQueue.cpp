#include "worker/RedisStreamJobQueue.h"
#include "logging/Logger.h"
#include <drogon/drogon.h>
#include <drogon/nosql/RedisClient.h>
#include <drogon/nosql/RedisException.h>
#include <drogon/nosql/RedisResult.h>
#include <chrono>
#include <cstdlib>

namespace cortex::worker {

using cortex::logging::Logger;

RedisStreamJobQueue::RedisStreamJobQueue(std::string redisClientName,
                                         std::string streamName,
                                         std::string groupName) noexcept
    : redisClientName_(std::move(redisClientName)),
      streamName_(std::move(streamName)),
      groupName_(std::move(groupName)),
      deadLetterStreamName_(streamName_ + ":dead") {}

long long RedisStreamJobQueue::resolveMaxBacklog() const noexcept {
    constexpr long long kDefaultMaxBacklog = 64;

    const char* raw = std::getenv("JOB_MAX_BACKLOG");
    if (!raw) {
        return kDefaultMaxBacklog;
    }

    try {
        const long long parsed = std::stoll(raw);
        if (parsed <= 0) {
            Logger::instance().warn("Invalid JOB_MAX_BACKLOG (<= 0), using default 64");
            return kDefaultMaxBacklog;
        }
        return parsed;
    } catch (...) {
        Logger::instance().warn("Failed to parse JOB_MAX_BACKLOG, using default 64");
        return kDefaultMaxBacklog;
    }
}

bool RedisStreamJobQueue::isBackpressured() noexcept {
    try {
        const auto start = std::chrono::steady_clock::now();
        auto client = drogon::app().getRedisClient(redisClientName_);
        if (!client) {
            return false;
        }

        const auto lockWaitStart = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> commandLock(commandMutex_);
        const auto lockWaitEnd = std::chrono::steady_clock::now();
        const auto cmdStart = std::chrono::steady_clock::now();
        const auto parser = [this](const drogon::nosql::RedisResult& result) -> long long {
            if (result.isNil()) {
                return 0;
            }

            long long pending = 0;
            long long lag = 0;

            const auto groups = result.asArray();
            for (const auto& groupEntry : groups) {
                const auto kv = groupEntry.asArray();
                std::string name;
                long long groupPending = 0;
                long long groupLag = 0;
                for (size_t i = 0; i + 1 < kv.size(); i += 2) {
                    const std::string key = kv[i].asString();
                    const auto& value = kv[i + 1];
                    if (key == "name") {
                        name = value.asString();
                    } else if (key == "pending") {
                        groupPending = value.asInteger();
                    } else if (key == "lag") {
                        if (!value.isNil()) {
                            groupLag = value.asInteger();
                        }
                    }
                }

                if (name == groupName_) {
                    pending = groupPending;
                    lag = groupLag;
                    break;
                }
            }

            return pending + lag;
        };

        const long long load = client->execCommandSync<long long>(
            parser,
            "XINFO GROUPS %s",
            streamName_.c_str());
        const auto cmdEnd = std::chrono::steady_clock::now();
        const auto end = std::chrono::steady_clock::now();

        const auto lockWaitMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            lockWaitEnd - lockWaitStart).count();
        const auto cmdMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            cmdEnd - cmdStart).count();
        const auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            end - start).count();
        if (lockWaitMs > 20 || cmdMs > 20 || totalMs > 20) {
            Logger::instance().warn(
                "backpressure_timing mutex_wait_ms=" + std::to_string(lockWaitMs) +
                " xinfo_ms=" + std::to_string(cmdMs) +
                " total_ms=" + std::to_string(totalMs));
        }

        const long long maxBacklog = resolveMaxBacklog();
        if (load >= maxBacklog) {
            Logger::instance().warn(
                "Backpressure active: group load=" + std::to_string(load) +
                " reached limit=" + std::to_string(maxBacklog));
            return true;
        }
        return false;
    } catch (const std::exception& e) {
        Logger::instance().warn(
            "Backpressure check failed, proceeding without rejection: " + std::string(e.what()));
        return false;
    }
}

bool RedisStreamJobQueue::ensureConsumerGroup() noexcept {
    if (groupEnsured_.load()) {
        return true;
    }

    std::lock_guard<std::mutex> lock(ensureMutex_);
    if (groupEnsured_.load()) {
        return true;
    }

    try {
        const auto start = std::chrono::steady_clock::now();
        auto client = drogon::app().getRedisClient(redisClientName_);
        if (!client) {
            Logger::instance().error("Redis client unavailable while creating consumer group");
            return false;
        }
        const auto lockWaitStart = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> commandLock(commandMutex_);
        const auto lockWaitEnd = std::chrono::steady_clock::now();
        const auto cmdStart = std::chrono::steady_clock::now();
        client->execCommandSync<long long>(
            [](const drogon::nosql::RedisResult&) {
                return 1LL;
            },
            "XGROUP CREATE %s %s %s MKSTREAM",
            streamName_.c_str(),
            groupName_.c_str(),
            "$");
        const auto cmdEnd = std::chrono::steady_clock::now();
        const auto end = std::chrono::steady_clock::now();

        Logger::instance().info(
            "ensure_group_timing mutex_wait_ms=" +
            std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                lockWaitEnd - lockWaitStart).count()) +
            " xgroup_create_ms=" +
            std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                cmdEnd - cmdStart).count()) +
            " total_ms=" +
            std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                end - start).count()));

        groupEnsured_.store(true);
        Logger::instance().info("Redis consumer group created: " + groupName_);
        return true;
    } catch (const drogon::nosql::RedisException& e) {
        const std::string err = e.what();
        if (err.find("BUSYGROUP") != std::string::npos) {
            groupEnsured_.store(true);
            return true;
        }
        Logger::instance().error("Failed to ensure Redis consumer group: " + err);
        return false;
    } catch (const std::exception& e) {
        Logger::instance().error("Failed to ensure Redis consumer group: " + std::string(e.what()));
        return false;
    }
}

bool RedisStreamJobQueue::publishJob(const std::string& jobId, int attempt) noexcept {
    try {
        const auto publishStart = std::chrono::steady_clock::now();

        if (!ensureConsumerGroup()) {
            return false;
        }

        if (attempt <= 0 && isBackpressured()) {
            return false;
        }

        const auto acquireStart = std::chrono::steady_clock::now();
        auto client = drogon::app().getRedisClient(redisClientName_);
        const auto acquireEnd = std::chrono::steady_clock::now();
        if (!client) {
            Logger::instance().error("Redis client unavailable while publishing job");
            return false;
        }

        const auto lockWaitStart = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> commandLock(commandMutex_);
        const auto lockWaitEnd = std::chrono::steady_clock::now();

        const auto xaddStart = std::chrono::steady_clock::now();
        const std::string messageId = client->execCommandSync<std::string>(
            [](const drogon::nosql::RedisResult& r) {
                return r.asString();
            },
            "XADD %s * job_id %s attempt %d",
            streamName_.c_str(),
            jobId.c_str(),
            attempt);
        const auto xaddEnd = std::chrono::steady_clock::now();
        const auto publishEnd = std::chrono::steady_clock::now();

        const auto acquireMs = std::chrono::duration_cast<std::chrono::milliseconds>(acquireEnd - acquireStart).count();
        const auto lockWaitMs = std::chrono::duration_cast<std::chrono::milliseconds>(lockWaitEnd - lockWaitStart).count();
        const auto xaddMs = std::chrono::duration_cast<std::chrono::milliseconds>(xaddEnd - xaddStart).count();
        const auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(publishEnd - publishStart).count();

        Logger::instance().info(
            "publish_timing jobId=" + jobId +
            " attempt=" + std::to_string(attempt) +
            " client_acquire_ms=" + std::to_string(acquireMs) +
            " mutex_wait_ms=" + std::to_string(lockWaitMs) +
            " xadd_ms=" + std::to_string(xaddMs) +
            " total_ms=" + std::to_string(totalMs));

        Logger::instance().info(
            "Redis XADD published job " + jobId +
            " attempt=" + std::to_string(attempt) +
            " as message " + messageId);
        return true;
    } catch (const std::exception& e) {
        Logger::instance().error("Failed to publish job to Redis stream: " + std::string(e.what()));
        return false;
    }
}

std::optional<StreamJobMessage> RedisStreamJobQueue::parseStreamMessage(
    const drogon::nosql::RedisResult& result) const
{
    if (result.isNil()) {
        return std::nullopt;
    }

    auto streams = result.asArray();
    if (streams.empty()) {
        return std::nullopt;
    }

    auto streamEnvelope = streams.front().asArray();
    if (streamEnvelope.size() < 2) {
        return std::nullopt;
    }

    auto messages = streamEnvelope[1].asArray();
    if (messages.empty()) {
        return std::nullopt;
    }

    auto messageEnvelope = messages.front().asArray();
    if (messageEnvelope.size() < 2) {
        return std::nullopt;
    }

    StreamJobMessage parsed;
    parsed.streamId = messageEnvelope[0].asString();

    auto fields = messageEnvelope[1].asArray();
    for (size_t i = 0; i + 1 < fields.size(); i += 2) {
        const std::string key = fields[i].asString();
        const std::string value = fields[i + 1].asString();
        if (key == "job_id") {
            parsed.jobId = value;
        } else if (key == "attempt") {
            try {
                parsed.attempt = std::stoi(value);
            } catch (...) {
                parsed.attempt = 0;
            }
        }
    }

    if (parsed.jobId.empty()) {
        return std::nullopt;
    }

    return parsed;
}

std::optional<StreamJobMessage> RedisStreamJobQueue::consumeNext(const std::string& consumerName,
                                                                 int blockMs) noexcept {
    (void)blockMs;
    try {
        if (!ensureConsumerGroup()) {
            return std::nullopt;
        }

        auto client = drogon::app().getRedisClient(redisClientName_);
        if (!client) {
            Logger::instance().error("Redis client unavailable while consuming jobs");
            return std::nullopt;
        }
        std::lock_guard<std::mutex> commandLock(commandMutex_);

        // First read pending entries assigned to this consumer (recovery path).
        std::function<std::optional<StreamJobMessage>(const drogon::nosql::RedisResult&)> parseFn =
            [this](const drogon::nosql::RedisResult& r) {
                return parseStreamMessage(r);
            };

        auto pending = client->execCommandSync<std::optional<StreamJobMessage>>(
            std::move(parseFn),
            "XREADGROUP GROUP %s %s COUNT 1 STREAMS %s %s",
            groupName_.c_str(),
            consumerName.c_str(),
            streamName_.c_str(),
            "0");

        if (pending.has_value()) {
            return pending;
        }

        std::function<std::optional<StreamJobMessage>(const drogon::nosql::RedisResult&)> parseFnFresh =
            [this](const drogon::nosql::RedisResult& r) {
                return parseStreamMessage(r);
            };

        // Then read new entries for this group.
        // Omit BLOCK for a truly non-blocking read; BLOCK 0 means block forever in Redis.
        // Worker loop handles polling cadence.
        auto fresh = client->execCommandSync<std::optional<StreamJobMessage>>(
            std::move(parseFnFresh),
            "XREADGROUP GROUP %s %s COUNT 1 STREAMS %s %s",
            groupName_.c_str(),
            consumerName.c_str(),
            streamName_.c_str(),
            ">"
        );

        return fresh;
    } catch (const std::exception& e) {
        Logger::instance().error("Redis consume error: " + std::string(e.what()));
        return std::nullopt;
    }
}

bool RedisStreamJobQueue::ack(const std::string& streamId) noexcept {
    try {
        auto client = drogon::app().getRedisClient(redisClientName_);
        if (!client) {
            Logger::instance().error("Redis client unavailable while acknowledging message");
            return false;
        }
        std::lock_guard<std::mutex> commandLock(commandMutex_);
        const long long acked = client->execCommandSync<long long>(
            [](const drogon::nosql::RedisResult& r) {
                return r.asInteger();
            },
            "XACK %s %s %s",
            streamName_.c_str(),
            groupName_.c_str(),
            streamId.c_str());

        return acked > 0;
    } catch (const std::exception& e) {
        Logger::instance().error("Redis ack error: " + std::string(e.what()));
        return false;
    }
}

bool RedisStreamJobQueue::publishDeadLetter(const StreamJobMessage& message,
                                            const std::string& reason) noexcept {
    try {
        auto client = drogon::app().getRedisClient(redisClientName_);
        if (!client) {
            Logger::instance().error("Redis client unavailable while publishing dead-letter");
            return false;
        }

        std::lock_guard<std::mutex> commandLock(commandMutex_);
        const std::string messageId = client->execCommandSync<std::string>(
            [](const drogon::nosql::RedisResult& r) {
                return r.asString();
            },
            "XADD %s * job_id %s attempt %d original_stream_id %s reason %s",
            deadLetterStreamName_.c_str(),
            message.jobId.c_str(),
            message.attempt,
            message.streamId.c_str(),
            reason.c_str());

        Logger::instance().warn(
            "Published dead-letter message for job " + message.jobId +
            " attempt=" + std::to_string(message.attempt) +
            " dead_letter_id=" + messageId);
        return true;
    } catch (const std::exception& e) {
        Logger::instance().error("Failed to publish dead-letter message: " + std::string(e.what()));
        return false;
    }
}

} // namespace cortex::worker
