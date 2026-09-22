#include "proxima/diagnostics/network_measurement_engine.hpp"

#include <algorithm>
#include <cmath>

namespace proxima::diagnostics {

namespace {
double clampScore(const double value) {
    return std::clamp(value, 0.0, 100.0);
}
}

NetworkMeasurementEngine::NetworkMeasurementEngine(const std::size_t rollingWindow)
    : statistics_(rollingWindow) {}

void NetworkMeasurementEngine::start(const core::MeasurementMode mode) {
    std::lock_guard lock(mutex_);
    mode_ = mode;
    running_ = true;
}

void NetworkMeasurementEngine::stop() {
    std::lock_guard lock(mutex_);
    running_ = false;
    mode_ = core::MeasurementMode::Idle;
}

void NetworkMeasurementEngine::reset() {
    std::lock_guard lock(mutex_);
    statistics_.reset();
    loss_.reset();
    lastUpdated_ = {};
}

void NetworkMeasurementEngine::record(const core::NetworkSample& sample) {
    std::lock_guard lock(mutex_);
    loss_.record(sample.successful());
    if (sample.rttMs.has_value()) {
        statistics_.add(*sample.rttMs);
        lastUpdated_ = sample.timestamp;
    }
}

core::MeasurementMode NetworkMeasurementEngine::mode() const noexcept {
    std::lock_guard lock(mutex_);
    return mode_;
}

bool NetworkMeasurementEngine::running() const noexcept {
    std::lock_guard lock(mutex_);
    return running_;
}

core::RouteMetrics NetworkMeasurementEngine::snapshot(
    const std::string& routeId,
    const core::RouteType routeType) const {
    std::lock_guard lock(mutex_);
    core::RouteMetrics result;
    result.routeId = routeId;
    result.routeType = routeType;
    result.sampleCount = statistics_.size();
    result.lastUpdated = lastUpdated_;
    result.packetLossPct = loss_.lossPercent();

    if (const auto current = statistics_.values(); !current.empty()) {
        result.rttCurrentMs = current.back();
        result.rttMinMs = *statistics_.min();
        result.rttAvgMs = *statistics_.mean();
        result.rttMaxMs = *statistics_.max();
        result.jitterMs = JitterCalculator::calculate(current);
        result.stabilityScore = clampScore(100.0 - result.jitterMs * 2.0 - result.packetLossPct * 8.0);
        result.health = result.packetLossPct >= 10.0
            ? core::RouteHealth::Degraded
            : (result.stabilityScore >= 75.0 ? core::RouteHealth::Healthy : core::RouteHealth::Degraded);
    } else if (loss_.transmitted() > 0) {
        result.health = core::RouteHealth::Offline;
        result.stabilityScore = 0.0;
    } else {
        result.health = core::RouteHealth::Unknown;
    }
    return result;
}

} // namespace proxima::diagnostics
