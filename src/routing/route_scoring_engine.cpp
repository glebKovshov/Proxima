#include "proxima/routing/route_scoring_engine.hpp"

#include <algorithm>
#include <utility>

namespace proxima::routing {

namespace {
double normalizedPositive(const double value, const double ideal, const double maximum) {
    if (value <= ideal) return 100.0;
    if (maximum <= ideal) return 0.0;
    return std::clamp(100.0 * (maximum - value) / (maximum - ideal), 0.0, 100.0);
}

double inverseNormalized(const double value, const double maximum) {
    if (maximum <= 0.0) return 0.0;
    return std::clamp(100.0 * (1.0 - value / maximum), 0.0, 100.0);
}
}

RouteScoringEngine::RouteScoringEngine(ScoringParameters parameters)
    : parameters_(std::move(parameters)) {}

ScoreResult RouteScoringEngine::score(const core::RouteMetrics& metrics) const {
    ScoreResult result;
    if (metrics.sampleCount == 0) {
        if (metrics.packetLossPct >= 100.0) {
            result.health = core::RouteHealth::Offline;
            result.label = "Offline";
        }
        return result;
    }

    const double rttScore = normalizedPositive(
        metrics.rttAvgMs, parameters_.idealRttMs, parameters_.maxRttMs);
    const double jitterScore = inverseNormalized(metrics.jitterMs, parameters_.maxJitterMs);
    const double lossScore = inverseNormalized(metrics.packetLossPct, parameters_.maxPacketLossPct);
    const double stabilityScore = metrics.stabilityScore > 0.0
        ? std::clamp(metrics.stabilityScore, 0.0, 100.0)
        : std::clamp(100.0 - metrics.jitterMs * 2.0 - metrics.packetLossPct * 8.0, 0.0, 100.0);
    const double sampleConfidence = parameters_.confidenceSampleCount == 0
        ? 1.0
        : std::clamp(
            static_cast<double>(metrics.sampleCount) /
                static_cast<double>(parameters_.confidenceSampleCount),
            0.0,
            1.0);

    result.qualityScore = std::clamp(
        rttScore * parameters_.rttWeight +
            jitterScore * parameters_.jitterWeight +
            lossScore * parameters_.packetLossWeight +
            stabilityScore * parameters_.stabilityWeight,
        0.0,
        100.0);
    result.confidence = sampleConfidence;
    if (metrics.packetLossPct >= 100.0 || metrics.sampleCount == 0) {
        result.health = core::RouteHealth::Offline;
        result.label = "Offline";
    } else if (result.qualityScore >= parameters_.excellentThreshold) {
        result.health = core::RouteHealth::Healthy;
        result.label = "Excellent";
    } else if (result.qualityScore >= parameters_.goodThreshold) {
        result.health = core::RouteHealth::Healthy;
        result.label = "Good";
    } else if (result.qualityScore >= parameters_.unstableThreshold) {
        result.health = core::RouteHealth::Degraded;
        result.label = "Unstable";
    } else {
        result.health = core::RouteHealth::Offline;
        result.label = "Poor";
    }
    return result;
}

const ScoringParameters& RouteScoringEngine::parameters() const noexcept { return parameters_; }

} // namespace proxima::routing
