#pragma once

#include "proxima/core/domain.hpp"
#include "proxima/diagnostics/statistics.hpp"

#include <chrono>
#include <cstddef>
#include <mutex>
#include <optional>

namespace proxima::diagnostics {

class NetworkMeasurementEngine {
public:
    explicit NetworkMeasurementEngine(std::size_t rollingWindow = 120);

    void start(core::MeasurementMode mode);
    void stop();
    void reset();
    void record(const core::NetworkSample& sample);

    [[nodiscard]] core::MeasurementMode mode() const noexcept;
    [[nodiscard]] bool running() const noexcept;
    [[nodiscard]] core::RouteMetrics snapshot(
        const std::string& routeId = "direct",
        core::RouteType routeType = core::RouteType::Direct) const;

private:
    mutable std::mutex mutex_;
    RollingStatistics statistics_;
    PacketLossAnalyzer loss_;
    core::MeasurementMode mode_{core::MeasurementMode::Idle};
    bool running_{false};
    std::chrono::system_clock::time_point lastUpdated_{};
};

} // namespace proxima::diagnostics
