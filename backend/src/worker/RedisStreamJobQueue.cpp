#include "worker/RedisStreamJobQueue.h"
#include "logging/Logger.h"
#include <drogon/drogon.h>
#include <drogon/nosql/RedisClient.h>
#include <drogon/nosql/RedisException.h>
#include <drogon/nosql/RedisResult.h>

namespace cortex::worker {

using cortex::logging::Logger;

RedisStreamJobQueue::RedisStreamJobQueue(std::string redisClientName,
                                         std::string streamName,
                                         std::string groupName) noexcept
    : redisClientName_(std::move(redisClientName)),
      streamName_(std::move(streamName)),
      groupName_(std::move(groupName)) {}

bool RedisStreamJobQueue::ensureConsumerGroup() noexcept {
    if (groupEnsured_.load()) {
        return true;
    }

    std::lock_guard<std::mutex> lock(ensureMutex_);
    if (groupEnsured_.load()) {
        return true;
    }

    try {
        auto client = drogon::app().getRedisClient(redisClientName_);
        if (!client) {
            Logger::instance().error("Fast Redis client unavailable while creating consumer group");
            return false;
        }
        client->execCommandSync<long long>(
            [](const drogon::nosql::RedisResult&) {
                return 1LL;
            },
            "XGROUP CREATE %s %s %s MKSTREAM",
            streamName_.c_str(),
            groupName_.c_str(),
            "$");

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

bool RedisStreamJobQueue::publishJob(const std::string& jobId) noexcept {
    try {
        if (!ensureConsumerGroup()) {
            return false;
        }

        auto client = drogon::app().getRedisClient(redisClientName_);
        if (!client) {
            Logger::instance().error("Fast Redis client unavailable while publishing job");
            return false;
        }
        const std::string messageId = client->execCommandSync<std::string>(
            [](const drogon::nosql::RedisResult& r) {
                return r.asString();
            },
            "XADD %s * job_id %s",
            streamName_.c_str(),
            jobId.c_str());

        Logger::instance().info("Redis XADD published job " + jobId + " as message " + messageId);
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
            break;
        }
    }

    if (parsed.jobId.empty()) {
        return std::nullopt;
    }

    return parsed;
}

std::optional<StreamJobMessage> RedisStreamJobQueue::consumeNext(const std::string& consumerName,
                                                                 int blockMs) noexcept {
    try {
        if (!ensureConsumerGroup()) {
            return std::nullopt;
        }

        auto client = drogon::app().getRedisClient(redisClientName_);
        if (!client) {
            Logger::instance().error("Fast Redis client unavailable while consuming jobs");
            return std::nullopt;
        }

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
        auto fresh = client->execCommandSync<std::optional<StreamJobMessage>>(
            std::move(parseFnFresh),
            "XREADGROUP GROUP %s %s COUNT 1 BLOCK %d STREAMS %s %s",
            groupName_.c_str(),
            consumerName.c_str(),
            blockMs,
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
            Logger::instance().error("Fast Redis client unavailable while acknowledging message");
            return false;
        }
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

} // namespace cortex::worker
