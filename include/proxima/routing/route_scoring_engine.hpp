#pragma once

#include "proxima/core/domain.hpp"

#include <cstddef>
#include <string>

namespace proxima::routing {

struct ScoringParameters {
    double rttWeight{0.45};
    double jitterWeight{0.20};
    double packetLossWeight{0.25};
    double stabilityWeight{0.10};
    double idealRttMs{20.0};
    double maxRttMs{200.0};
    double maxJitterMs{50.0};
    double maxPacketLossPct{10.0};
    std::size_t confidenceSampleCount{20};
    double excellentThreshold{90.0};
    double goodThreshold{75.0};
    double unstableThreshold{50.0};
};

struct ScoreResult {
    double qualityScore{0.0};
    double confidence{0.0};
    core::RouteHealth health{core::RouteHealth::Unknown};
    std::string label{"Unknown"};
};

class RouteScoringEngine {
public:
    explicit RouteScoringEngine(ScoringParameters parameters = {});

    [[nodiscard]] ScoreResult score(const core::RouteMetrics& metrics) const;
    [[nodiscard]] const ScoringParameters& parameters() const noexcept;

private:
    ScoringParameters parameters_;
};

} // namespace proxima::routing
