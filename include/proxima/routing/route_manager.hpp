#pragma once

#include "proxima/core/domain.hpp"
#include "proxima/routing/route_scoring_engine.hpp"

#include <optional>
#include <vector>

namespace proxima::routing {

class BestRouteSelector {
public:
    [[nodiscard]] std::optional<core::Route> select(const std::vector<core::Route>& routes) const;
};

class RouteManager {
public:
    explicit RouteManager(RouteScoringEngine scoringEngine = RouteScoringEngine(ScoringParameters{}));

    void setRoutes(std::vector<core::Route> routes);
    void updateMetrics(const core::RouteMetrics& metrics);
    [[nodiscard]] const std::vector<core::Route>& routes() const noexcept;
    [[nodiscard]] std::optional<core::Route> bestRoute() const;

private:
    RouteScoringEngine scoringEngine_;
    std::vector<core::Route> routes_;
};

} // namespace proxima::routing
