#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace cortex::observability {

struct TraceContext {
    std::string traceparent;
    std::string tracestate;

    bool empty() const noexcept {
        return traceparent.empty() && tracestate.empty();
    }
};

enum class SpanKind {
    Internal,
    Server,
    Client,
    Producer,
    Consumer
};

class ITraceSpan {
public:
    virtual ~ITraceSpan() = default;

    virtual void setAttribute(std::string_view key, std::string_view value) noexcept = 0;
    virtual void setAttribute(std::string_view key, std::int64_t value) noexcept = 0;
    virtual void setAttribute(std::string_view key, double value) noexcept = 0;
    virtual void setAttribute(std::string_view key, bool value) noexcept = 0;

    virtual void setStatusOk() noexcept = 0;
    virtual void setStatusError(std::string_view code) noexcept = 0;

    virtual std::optional<TraceContext> context() const noexcept = 0;
    virtual void end() noexcept = 0;
};

class ITraceScope {
public:
    virtual ~ITraceScope() = default;
};

class ITracing {
public:
    virtual ~ITracing() = default;

    virtual std::shared_ptr<ITraceSpan> startSpan(
        std::string_view name,
        SpanKind kind = SpanKind::Internal,
        const std::optional<TraceContext>& parent = std::nullopt) noexcept = 0;

    virtual std::unique_ptr<ITraceScope> activateSpan(
        const std::shared_ptr<ITraceSpan>& span) noexcept = 0;

    virtual std::optional<TraceContext> currentContext() const noexcept = 0;
};

} // namespace cortex::observability
