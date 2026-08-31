#include "observability/OpenTelemetryTracingService.h"

#include "logging/Logger.h"

#include <opentelemetry/context/runtime_context.h>
#include <opentelemetry/context/propagation/global_propagator.h>
#include <opentelemetry/context/propagation/text_map_propagator.h>
#include <opentelemetry/exporters/otlp/otlp_http_exporter_factory.h>
#include <opentelemetry/exporters/otlp/otlp_http_exporter_options.h>
#include <opentelemetry/nostd/shared_ptr.h>
#include <opentelemetry/sdk/resource/resource.h>
#include <opentelemetry/sdk/trace/batch_span_processor.h>
#include <opentelemetry/sdk/trace/tracer_provider.h>
#include <opentelemetry/trace/propagation/http_trace_context.h>
#include <opentelemetry/trace/provider.h>
#include <opentelemetry/trace/scope.h>
#include <opentelemetry/trace/span.h>
#include <opentelemetry/trace/tracer.h>

#include <atomic>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

namespace cortex::observability {
namespace {
namespace otel = opentelemetry;

std::string toHexLower(const uint8_t* data, std::size_t size) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out;
    out.resize(size * 2);

    for (std::size_t i = 0; i < size; ++i) {
        const uint8_t byte = data[i];
        out[i * 2] = kHex[(byte >> 4) & 0xF];
        out[i * 2 + 1] = kHex[byte & 0xF];
    }
    return out;
}

std::optional<TraceContext> spanContextToTraceContext(const otel::trace::SpanContext& spanContext) {
    if (!spanContext.IsValid()) {
        return std::nullopt;
    }

    const auto traceIdBytes = spanContext.trace_id().Id();
    const auto spanIdBytes = spanContext.span_id().Id();

    std::string traceparent = "00-";
    traceparent += toHexLower(traceIdBytes.data(), traceIdBytes.size());
    traceparent += "-";
    traceparent += toHexLower(spanIdBytes.data(), spanIdBytes.size());
    traceparent += "-";

    const uint8_t flags = spanContext.trace_flags().flags();
    traceparent += toHexLower(&flags, 1);

    TraceContext out;
    out.traceparent = std::move(traceparent);

    auto traceState = spanContext.trace_state();
    if (traceState && !traceState->Empty()) {
        out.tracestate = traceState->ToHeader();
    }
    return out;
}

class MapCarrier : public otel::context::propagation::TextMapCarrier {
public:
    explicit MapCarrier(std::unordered_map<std::string, std::string>* fields) noexcept
        : fields_(fields) {}

    otel::nostd::string_view Get(otel::nostd::string_view key) const noexcept override {
        if (!fields_) {
            return "";
        }

        auto it = fields_->find(std::string(key.data(), key.size()));
        if (it == fields_->end()) {
            return "";
        }

        return it->second;
    }

    void Set(otel::nostd::string_view key, otel::nostd::string_view value) noexcept override {
        if (!fields_) {
            return;
        }

        (*fields_)[std::string(key.data(), key.size())] =
            std::string(value.data(), value.size());
    }

private:
    std::unordered_map<std::string, std::string>* fields_;
};

otel::trace::SpanKind toOtelSpanKind(SpanKind kind) noexcept {
    using otel::trace::SpanKind;
    switch (kind) {
    case cortex::observability::SpanKind::Server:
        return SpanKind::kServer;
    case cortex::observability::SpanKind::Client:
        return SpanKind::kClient;
    case cortex::observability::SpanKind::Producer:
        return SpanKind::kProducer;
    case cortex::observability::SpanKind::Consumer:
        return SpanKind::kConsumer;
    case cortex::observability::SpanKind::Internal:
    default:
        return SpanKind::kInternal;
    }
}

const otel::nostd::shared_ptr<otel::context::propagation::TextMapPropagator>&
propagator() noexcept {
    static otel::nostd::shared_ptr<otel::context::propagation::TextMapPropagator> p(
        new otel::trace::propagation::HttpTraceContext());
    return p;
}

std::string getenvOrDefault(const char* key, const char* fallback) {
    if (const char* v = std::getenv(key)) {
        if (*v != '\0') {
            return std::string(v);
        }
    }
    return std::string(fallback);
}

class OpenTelemetrySpan final : public ITraceSpan {
public:
    explicit OpenTelemetrySpan(otel::nostd::shared_ptr<otel::trace::Span> span) noexcept
        : span_(std::move(span)) {}

    void setAttribute(std::string_view key, std::string_view value) noexcept override {
        if (!span_) {
            return;
        }
        span_->SetAttribute(std::string(key), std::string(value));
    }

    void setAttribute(std::string_view key, std::int64_t value) noexcept override {
        if (!span_) {
            return;
        }
        span_->SetAttribute(std::string(key), value);
    }

    void setAttribute(std::string_view key, double value) noexcept override {
        if (!span_) {
            return;
        }
        span_->SetAttribute(std::string(key), value);
    }

    void setAttribute(std::string_view key, bool value) noexcept override {
        if (!span_) {
            return;
        }
        span_->SetAttribute(std::string(key), value);
    }

    void setStatusOk() noexcept override {
        if (!span_) {
            return;
        }
        span_->SetStatus(otel::trace::StatusCode::kOk);
    }

    void setStatusError(std::string_view code) noexcept override {
        if (!span_) {
            return;
        }
        span_->SetStatus(otel::trace::StatusCode::kError, std::string(code));
    }

    std::optional<TraceContext> context() const noexcept override {
        if (!span_) {
            return std::nullopt;
        }
        return spanContextToTraceContext(span_->GetContext());
    }

    void end() noexcept override {
        if (!span_) {
            return;
        }
        bool expected = false;
        if (ended_.compare_exchange_strong(expected, true)) {
            span_->End();
        }
    }

    otel::nostd::shared_ptr<otel::trace::Span> rawSpan() const noexcept {
        return span_;
    }

private:
    otel::nostd::shared_ptr<otel::trace::Span> span_;
    mutable std::atomic<bool> ended_{false};
};

class OpenTelemetryScope final : public ITraceScope {
public:
    explicit OpenTelemetryScope(otel::nostd::shared_ptr<otel::trace::Span> span) noexcept
        : scope_(span) {}

private:
    otel::trace::Scope scope_;
};

class NoopTraceSpan final : public ITraceSpan {
public:
    void setAttribute(std::string_view, std::string_view) noexcept override {}
    void setAttribute(std::string_view, std::int64_t) noexcept override {}
    void setAttribute(std::string_view, double) noexcept override {}
    void setAttribute(std::string_view, bool) noexcept override {}
    void setStatusOk() noexcept override {}
    void setStatusError(std::string_view) noexcept override {}
    std::optional<TraceContext> context() const noexcept override { return std::nullopt; }
    void end() noexcept override {}
};

class NoopTraceScope final : public ITraceScope {};

} // namespace

OpenTelemetryTracingService::OpenTelemetryTracingService() noexcept {
    try {
        otel::exporter::otlp::OtlpHttpExporterOptions opts;
        opts.url = getenvOrDefault("OTEL_EXPORTER_OTLP_TRACES_ENDPOINT", "http://otel-collector:4318/v1/traces");
        opts.content_type = otel::exporter::otlp::HttpRequestContentType::kJson;

        auto exporter =
            otel::exporter::otlp::OtlpHttpExporterFactory::Create(opts);
        auto processor =
            std::make_unique<otel::sdk::trace::BatchSpanProcessor>(
                std::move(exporter),
                otel::sdk::trace::BatchSpanProcessorOptions{});

        const std::string serviceName = getenvOrDefault("OTEL_SERVICE_NAME", "cortex-backend");
        const std::string serviceEnv = getenvOrDefault("OTEL_RESOURCE_ENVIRONMENT", "development");

        auto resource = otel::sdk::resource::Resource::Create({
            {"service.name", serviceName},
            {"deployment.environment", serviceEnv}
        });

        provider_ = std::make_shared<otel::sdk::trace::TracerProvider>(
            std::move(processor),
            resource);

        otel::nostd::shared_ptr<otel::trace::TracerProvider> apiProvider(
            std::static_pointer_cast<otel::trace::TracerProvider>(provider_));
        otel::trace::Provider::SetTracerProvider(apiProvider);
        otel::context::propagation::GlobalTextMapPropagator::SetGlobalPropagator(
            propagator());

        tracer_ = provider_->GetTracer("cortex.tracing", "1.0.0");
        enabled_ = true;
        cortex::logging::Logger::instance().info(
            "OpenTelemetry tracing initialized with OTLP endpoint: " + opts.url);
    } catch (const std::exception& e) {
        enabled_ = false;
        cortex::logging::Logger::instance().error(
            std::string("Failed to initialize OpenTelemetry tracing: ") + e.what());
    } catch (...) {
        enabled_ = false;
        cortex::logging::Logger::instance().error("Failed to initialize OpenTelemetry tracing");
    }
}

OpenTelemetryTracingService::~OpenTelemetryTracingService() noexcept {
    try {
        if (provider_) {
            provider_->ForceFlush();
        }
    } catch (...) {
    }
}

std::shared_ptr<ITraceSpan> OpenTelemetryTracingService::startSpan(
    std::string_view name,
    SpanKind kind,
    const std::optional<TraceContext>& parent) noexcept {
    if (!enabled_ || !tracer_) {
        return std::make_shared<NoopTraceSpan>();
    }

    try {
        otel::trace::StartSpanOptions options;
        options.kind = toOtelSpanKind(kind);

        if (parent.has_value()) {
            std::unordered_map<std::string, std::string> fields;
            if (!parent->traceparent.empty()) {
                fields["traceparent"] = parent->traceparent;
            }
            if (!parent->tracestate.empty()) {
                fields["tracestate"] = parent->tracestate;
            }

            MapCarrier carrier(&fields);
            auto parentContext = otel::context::Context{};
            const auto extracted = propagator()->Extract(carrier, parentContext);
            options.parent = extracted;
        }

        auto span = tracer_->StartSpan(std::string(name), options);
        return std::make_shared<OpenTelemetrySpan>(std::move(span));
    } catch (...) {
        return std::make_shared<NoopTraceSpan>();
    }
}

std::unique_ptr<ITraceScope> OpenTelemetryTracingService::activateSpan(
    const std::shared_ptr<ITraceSpan>& span) noexcept {
    if (!enabled_ || !span) {
        return std::make_unique<NoopTraceScope>();
    }

    auto concrete = std::dynamic_pointer_cast<OpenTelemetrySpan>(span);
    if (!concrete) {
        return std::make_unique<NoopTraceScope>();
    }

    try {
        return std::make_unique<OpenTelemetryScope>(concrete->rawSpan());
    } catch (...) {
        return std::make_unique<NoopTraceScope>();
    }
}

std::optional<TraceContext> OpenTelemetryTracingService::currentContext() const noexcept {
    if (!enabled_) {
        return std::nullopt;
    }

    try {
        auto currentSpan = otel::trace::Tracer::GetCurrentSpan();
        if (!currentSpan) {
            return std::nullopt;
        }
        return spanContextToTraceContext(currentSpan->GetContext());
    } catch (...) {
        return std::nullopt;
    }
}

} // namespace cortex::observability
