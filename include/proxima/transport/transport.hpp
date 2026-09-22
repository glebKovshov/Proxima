#pragma once

#include "proxima/core/domain.hpp"
#include "proxima/diagnostics/network_measurement_engine.hpp"

#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

namespace proxima::diagnostics { class PingProbe; }

namespace proxima::transport {

class ITransport {
public:
    virtual ~ITransport() = default;
    virtual bool start() = 0;
    virtual void stop() = 0;
    [[nodiscard]] virtual std::optional<core::NetworkSample> measure(
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) = 0;
    [[nodiscard]] virtual core::RouteMetrics metrics() const = 0;
    [[nodiscard]] virtual core::RouteHealth health() const = 0;
};

class DirectTransport final : public ITransport {
public:
    DirectTransport(
        std::string routeId,
        std::string endpoint,
        std::shared_ptr<diagnostics::PingProbe> probe,
        std::size_t rollingWindow = 120);

    bool start() override;
    void stop() override;
    [[nodiscard]] std::optional<core::NetworkSample> measure(
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) override;
    [[nodiscard]] core::RouteMetrics metrics() const override;
    [[nodiscard]] core::RouteHealth health() const override;
    [[nodiscard]] std::string endpoint() const;
    void setEndpoint(std::string endpoint);
    void resetMetrics();

private:
    std::string routeId_;
    std::string endpoint_;
    std::shared_ptr<diagnostics::PingProbe> probe_;
    std::unique_ptr<diagnostics::NetworkMeasurementEngine> engine_;
    mutable std::mutex mutex_;
    bool running_{false};
};

class RelayTransportStub final : public ITransport {
public:
    explicit RelayTransportStub(core::RelayRouteMetadata metadata);

    bool start() override;
    void stop() override;
    [[nodiscard]] std::optional<core::NetworkSample> measure(
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000)) override;
    [[nodiscard]] core::RouteMetrics metrics() const override;
    [[nodiscard]] core::RouteHealth health() const override;
    [[nodiscard]] const core::RelayRouteMetadata& metadata() const noexcept;

private:
    core::RelayRouteMetadata metadata_;
    core::RouteMetrics metrics_;
};

} // namespace proxima::transport
