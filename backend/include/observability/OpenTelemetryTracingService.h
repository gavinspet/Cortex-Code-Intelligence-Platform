#pragma once

#include "observability/ITracing.h"

#include <memory>
#include <string>
#include <opentelemetry/nostd/shared_ptr.h>
#include <opentelemetry/sdk/trace/tracer_provider.h>
#include <opentelemetry/trace/tracer.h>

namespace cortex::observability {

class OpenTelemetryTracingService final : public ITracing {
public:
    OpenTelemetryTracingService() noexcept;
    ~OpenTelemetryTracingService() noexcept override;

    std::shared_ptr<ITraceSpan> startSpan(
        std::string_view name,
        SpanKind kind = SpanKind::Internal,
        const std::optional<TraceContext>& parent = std::nullopt) noexcept override;

    std::unique_ptr<ITraceScope> activateSpan(
        const std::shared_ptr<ITraceSpan>& span) noexcept override;

    std::optional<TraceContext> currentContext() const noexcept override;

private:
    std::shared_ptr<opentelemetry::sdk::trace::TracerProvider> provider_;
    opentelemetry::nostd::shared_ptr<opentelemetry::trace::Tracer> tracer_;
    bool enabled_{false};
};

} // namespace cortex::observability
