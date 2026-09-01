#include "worker/RedisStreamJobQueue.h"
#include "logging/Logger.h"
#include <drogon/drogon.h>
#include <drogon/nosql/RedisClient.h>
#include <drogon/nosql/RedisException.h>
#include <drogon/nosql/RedisResult.h>
#include <chrono>
#include <cstdlib>
#include <cstdint>

namespace cortex::worker {

using cortex::logging::Logger;

namespace {

std::string resolveStorageBackend() {
    if (const char* value = std::getenv("STORAGE_BACKEND")) {
        if (*value != '\0') {
            return std::string(value);
        }
    }
    return "inmemory";
}

template <typename T>
std::optional<T> readIntegerField(const std::vector<drogon::nosql::RedisResult>& kv,
                                  const std::string& fieldName) {
    for (size_t i = 0; i + 1 < kv.size(); i += 2) {
        if (kv[i].asString() == fieldName) {
            if (kv[i + 1].isNil()) {
                return std::nullopt;
            }
            return static_cast<T>(kv[i + 1].asInteger());
        }
    }
    return std::nullopt;
}

std::optional<std::string> readStringField(const std::vector<drogon::nosql::RedisResult>& kv,
                                           const std::string& fieldName) {
    for (size_t i = 0; i + 1 < kv.size(); i += 2) {
        if (kv[i].asString() == fieldName) {
            if (kv[i + 1].isNil()) {
                return std::nullopt;
            }
            return kv[i + 1].asString();
        }
    }
    return std::nullopt;
}

void populateDispatchFields(const std::vector<drogon::nosql::RedisResult>& fields,
                            JobDispatchSnapshot& snapshot,
                            std::string* originalStreamId = nullptr,
                            std::string* reason = nullptr) {
    for (size_t i = 0; i + 1 < fields.size(); i += 2) {
        const std::string key = fields[i].asString();
        const std::string value = fields[i + 1].asString();
        if (key == "attempt") {
            try {
                snapshot.attempt = std::stoi(value);
            } catch (...) {
                snapshot.attempt = 0;
            }
        } else if (key == "traceparent") {
            snapshot.traceparent = value;
        } else if (key == "tracestate") {
            snapshot.tracestate = value;
        } else if (key == "original_stream_id" && originalStreamId) {
            *originalStreamId = value;
        } else if (key == "reason" && reason) {
            *reason = value;
        }
    }
}

} // namespace

RedisStreamJobQueue::RedisStreamJobQueue(std::string redisClientName,
                                         std::string streamName,
                                         std::string groupName,
                                         std::shared_ptr<cortex::observability::IMetrics> metrics,
                                         std::shared_ptr<cortex::observability::ITracing> tracing) noexcept
    : redisClientName_(std::move(redisClientName)),
      streamName_(std::move(streamName)),
      groupName_(std::move(groupName)),
      deadLetterStreamName_(streamName_ + ":dead"),
      metrics_(std::move(metrics)),
      tracing_(std::move(tracing)) {}

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

        if (metrics_) {
            metrics_->setJobsQueueDepth(static_cast<double>(load));
        }
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

bool RedisStreamJobQueue::publishJob(const std::string& jobId,
                                     int attempt,
                                     const std::optional<cortex::observability::TraceContext>& traceContext) noexcept {
    try {
        auto span = tracing_ ? tracing_->startSpan("redis.stream.xadd", cortex::observability::SpanKind::Producer, traceContext) : nullptr;
        if (span) {
            span->setAttribute("job.id", jobId);
            span->setAttribute("job.attempt", static_cast<std::int64_t>(attempt));
            span->setAttribute("storage.backend", resolveStorageBackend());
        }
        auto scope = (tracing_ && span) ? tracing_->activateSpan(span) : nullptr;

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
            "XADD %s * job_id %s attempt %d traceparent %s tracestate %s",
            streamName_.c_str(),
            jobId.c_str(),
            attempt,
            traceContext.has_value() ? traceContext->traceparent.c_str() : "",
            traceContext.has_value() ? traceContext->tracestate.c_str() : "");
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

        if (span) {
            span->setAttribute("redis.stream", streamName_);
            span->setAttribute("redis.message_id", messageId);
            span->setStatusOk();
            span->end();
        }
        return true;
    } catch (const std::exception& e) {
        if (tracing_) {
            auto errSpan = tracing_->startSpan("redis.stream.xadd", cortex::observability::SpanKind::Producer, traceContext);
            if (errSpan) {
                errSpan->setAttribute("job.id", jobId);
                errSpan->setAttribute("job.attempt", static_cast<std::int64_t>(attempt));
                errSpan->setStatusError("redis_xadd_failed");
                errSpan->end();
            }
        }
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
        } else if (key == "traceparent") {
            parsed.traceparent = value;
        } else if (key == "tracestate") {
            parsed.tracestate = value;
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
        auto span = tracing_ ? tracing_->startSpan("redis.stream.consume", cortex::observability::SpanKind::Consumer) : nullptr;
        if (span) {
            span->setAttribute("worker.id", consumerName);
            span->setAttribute("redis.stream", streamName_);
            span->setAttribute("redis.group", groupName_);
            span->setAttribute("storage.backend", resolveStorageBackend());
        }

        if (!ensureConsumerGroup()) {
            if (span) {
                span->setStatusError("redis_group_unavailable");
                span->end();
            }
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
            if (span) {
                span->setAttribute("job.id", pending->jobId);
                span->setAttribute("job.attempt", static_cast<std::int64_t>(pending->attempt));
                span->setAttribute("redis.message_id", pending->streamId);
                span->setStatusOk();
                span->end();
            }
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

        if (fresh.has_value() && span) {
            span->setAttribute("job.id", fresh->jobId);
            span->setAttribute("job.attempt", static_cast<std::int64_t>(fresh->attempt));
            span->setAttribute("redis.message_id", fresh->streamId);
        }
        if (span) {
            span->setStatusOk();
            span->end();
        }
        return fresh;
    } catch (const std::exception& e) {
        if (tracing_) {
            auto errSpan = tracing_->startSpan("redis.stream.consume", cortex::observability::SpanKind::Consumer);
            if (errSpan) {
                errSpan->setStatusError("redis_consume_error");
                errSpan->end();
            }
        }
        Logger::instance().error("Redis consume error: " + std::string(e.what()));
        return std::nullopt;
    }
}

bool RedisStreamJobQueue::ack(const std::string& streamId) noexcept {
    try {
        auto span = tracing_ ? tracing_->startSpan("redis.stream.xack", cortex::observability::SpanKind::Client) : nullptr;
        if (span) {
            span->setAttribute("redis.stream", streamName_);
            span->setAttribute("redis.group", groupName_);
            span->setAttribute("redis.message_id", streamId);
        }
        auto client = drogon::app().getRedisClient(redisClientName_);
        if (!client) {
            Logger::instance().error("Redis client unavailable while acknowledging message");
            if (span) {
                span->setStatusError("redis_client_unavailable");
                span->end();
            }
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

        const bool ok = acked > 0;
        if (span) {
            if (ok) {
                span->setStatusOk();
            } else {
                span->setStatusError("redis_xack_zero");
            }
            span->end();
        }
        return ok;
    } catch (const std::exception& e) {
        if (tracing_) {
            auto errSpan = tracing_->startSpan("redis.stream.xack", cortex::observability::SpanKind::Client);
            if (errSpan) {
                errSpan->setAttribute("redis.message_id", streamId);
                errSpan->setStatusError("redis_xack_error");
                errSpan->end();
            }
        }
        Logger::instance().error("Redis ack error: " + std::string(e.what()));
        return false;
    }
}

bool RedisStreamJobQueue::publishDeadLetter(const StreamJobMessage& message,
                                            const std::string& reason,
                                            const std::optional<cortex::observability::TraceContext>& traceContext) noexcept {
    try {
        std::optional<cortex::observability::TraceContext> parent = traceContext;
        if (!parent.has_value() && (!message.traceparent.empty() || !message.tracestate.empty())) {
            parent = cortex::observability::TraceContext{message.traceparent, message.tracestate};
        }

        auto span = tracing_ ? tracing_->startSpan("redis.stream.dead_letter.xadd", cortex::observability::SpanKind::Producer, parent) : nullptr;
        if (span) {
            span->setAttribute("job.id", message.jobId);
            span->setAttribute("job.attempt", static_cast<std::int64_t>(message.attempt));
            span->setAttribute("redis.stream", deadLetterStreamName_);
            span->setAttribute("dead_letter.reason", reason);
        }

        auto client = drogon::app().getRedisClient(redisClientName_);
        if (!client) {
            Logger::instance().error("Redis client unavailable while publishing dead-letter");
            if (span) {
                span->setStatusError("redis_client_unavailable");
                span->end();
            }
            return false;
        }

        std::lock_guard<std::mutex> commandLock(commandMutex_);
        const std::string messageId = client->execCommandSync<std::string>(
            [](const drogon::nosql::RedisResult& r) {
                return r.asString();
            },
            "XADD %s * job_id %s attempt %d original_stream_id %s reason %s traceparent %s tracestate %s",
            deadLetterStreamName_.c_str(),
            message.jobId.c_str(),
            message.attempt,
            message.streamId.c_str(),
            reason.c_str(),
            parent.has_value() ? parent->traceparent.c_str() : "",
            parent.has_value() ? parent->tracestate.c_str() : "");

        Logger::instance().warn(
            "Published dead-letter message for job " + message.jobId +
            " attempt=" + std::to_string(message.attempt) +
            " dead_letter_id=" + messageId);

        if (span) {
            span->setAttribute("redis.message_id", messageId);
            span->setStatusOk();
            span->end();
        }
        return true;
    } catch (const std::exception& e) {
        if (tracing_) {
            auto errSpan = tracing_->startSpan("redis.stream.dead_letter.xadd", cortex::observability::SpanKind::Producer);
            if (errSpan) {
                errSpan->setAttribute("job.id", message.jobId);
                errSpan->setAttribute("job.attempt", static_cast<std::int64_t>(message.attempt));
                errSpan->setStatusError("redis_dead_letter_failed");
                errSpan->end();
            }
        }
        Logger::instance().error("Failed to publish dead-letter message: " + std::string(e.what()));
        return false;
    }
}

std::optional<QueueGroupSnapshot> RedisStreamJobQueue::getQueueGroupSnapshot() noexcept {
    try {
        auto client = drogon::app().getRedisClient(redisClientName_);
        if (!client) {
            return std::nullopt;
        }

        std::lock_guard<std::mutex> commandLock(commandMutex_);

        QueueGroupSnapshot snapshot;
        snapshot.stream = streamName_;
        snapshot.consumerGroup = groupName_;

        auto groupSnapshot = client->execCommandSync<QueueGroupSnapshot>(
            [this, snapshot](const drogon::nosql::RedisResult& result) mutable {
                if (!result.isNil()) {
                    for (const auto& groupEntry : result.asArray()) {
                        const auto kv = groupEntry.asArray();
                        const auto name = readStringField(kv, "name");
                        if (!name.has_value() || *name != groupName_) {
                            continue;
                        }
                        snapshot.pending = readIntegerField<long long>(kv, "pending").value_or(0);
                        snapshot.lag = readIntegerField<long long>(kv, "lag").value_or(0);
                        break;
                    }
                }
                return snapshot;
            },
            "XINFO GROUPS %s",
            streamName_.c_str());

        auto consumers = client->execCommandSync<std::vector<QueueConsumerSnapshot>>(
            [](const drogon::nosql::RedisResult& result) {
                std::vector<QueueConsumerSnapshot> values;
                if (result.isNil()) {
                    return values;
                }
                for (const auto& consumerEntry : result.asArray()) {
                    const auto kv = consumerEntry.asArray();
                    QueueConsumerSnapshot item;
                    item.name = readStringField(kv, "name").value_or("");
                    item.pending = readIntegerField<long long>(kv, "pending").value_or(0);
                    item.idleMs = readIntegerField<long long>(kv, "idle").value_or(0);
                    if (!item.name.empty()) {
                        values.push_back(std::move(item));
                    }
                }
                return values;
            },
            "XINFO CONSUMERS %s %s",
            streamName_.c_str(),
            groupName_.c_str());

        groupSnapshot.consumers = std::move(consumers);
        return groupSnapshot;
    } catch (const std::exception& e) {
        Logger::instance().warn("Failed to query Redis queue snapshot: " + std::string(e.what()));
        return std::nullopt;
    }
}

std::optional<JobDispatchSnapshot> RedisStreamJobQueue::getJobDispatchSnapshot(const std::string& jobId) noexcept {
    try {
        auto client = drogon::app().getRedisClient(redisClientName_);
        if (!client) {
            return std::nullopt;
        }

        std::lock_guard<std::mutex> commandLock(commandMutex_);

        auto findLatestInStream = [&client, &jobId](const std::string& streamName,
                                                    bool deadLetter,
                                                    const std::string& queueStreamName) -> std::optional<JobDispatchSnapshot> {
            return client->execCommandSync<std::optional<JobDispatchSnapshot>>(
                [&jobId, deadLetter, &queueStreamName](const drogon::nosql::RedisResult& result) {
                    if (result.isNil()) {
                        return std::optional<JobDispatchSnapshot>{std::nullopt};
                    }

                    for (const auto& entry : result.asArray()) {
                        const auto envelope = entry.asArray();
                        if (envelope.size() < 2) {
                            continue;
                        }

                        const auto& entryId = envelope[0].asString();
                        const auto fields = envelope[1].asArray();
                        const auto messageJobId = readStringField(fields, "job_id");
                        if (!messageJobId.has_value() || *messageJobId != jobId) {
                            continue;
                        }

                        JobDispatchSnapshot snapshot;
                        snapshot.hasDispatchRecord = true;
                        snapshot.deadLettered = deadLetter;
                        snapshot.streamId = entryId;

                        std::string originalStreamId;
                        std::string reason;
                        populateDispatchFields(fields, snapshot, &originalStreamId, &reason);

                        if (deadLetter) {
                            snapshot.failureReason = reason;
                            if (!originalStreamId.empty()) {
                                snapshot.streamId = originalStreamId;
                            }
                        }

                        return std::optional<JobDispatchSnapshot>{snapshot};
                    }

                    return std::optional<JobDispatchSnapshot>{std::nullopt};
                },
                "XREVRANGE %s + -",
                streamName.c_str());
        };

        auto snapshot = findLatestInStream(deadLetterStreamName_, true, streamName_);
        if (!snapshot.has_value()) {
            snapshot = findLatestInStream(streamName_, false, streamName_);
        }
        if (!snapshot.has_value()) {
            return std::nullopt;
        }

        if (!snapshot->deadLettered && !snapshot->streamId.empty()) {
            auto pendingInfo = client->execCommandSync<std::optional<std::pair<std::string, long long>>>(
                [streamId = snapshot->streamId](const drogon::nosql::RedisResult& result) {
                    if (result.isNil()) {
                        return std::optional<std::pair<std::string, long long>>{std::nullopt};
                    }

                    for (const auto& entry : result.asArray()) {
                        const auto values = entry.asArray();
                        if (values.size() < 4) {
                            continue;
                        }
                        if (values[0].asString() != streamId) {
                            continue;
                        }
                        return std::optional<std::pair<std::string, long long>>{
                            std::make_pair(values[1].asString(), values[3].asInteger())};
                    }

                    return std::optional<std::pair<std::string, long long>>{std::nullopt};
                },
                "XPENDING %s %s - + 1000",
                streamName_.c_str(),
                groupName_.c_str());

            if (pendingInfo.has_value()) {
                snapshot->pending = true;
                snapshot->consumerName = pendingInfo->first;
                snapshot->deliveryCount = pendingInfo->second;
            }
        }

        return snapshot;
    } catch (const std::exception& e) {
        Logger::instance().warn(
            "Failed to query Redis dispatch snapshot for job " + jobId + ": " + std::string(e.what()));
        return std::nullopt;
    }
}

} // namespace cortex::worker
