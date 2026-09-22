#include "proxima/routing/route_manager.hpp"

#include <algorithm>
#include <utility>

namespace proxima::routing {

std::optional<core::Route> BestRouteSelector::select(const std::vector<core::Route>& routes) const {
    if (routes.empty()) return std::nullopt;
    const auto best = std::max_element(routes.begin(), routes.end(), [](const auto& left, const auto& right) {
        if (left.metrics.qualityScore != right.metrics.qualityScore) {
            return left.metrics.qualityScore < right.metrics.qualityScore;
        }
        return left.metrics.confidence < right.metrics.confidence;
    });
    return *best;
}

RouteManager::RouteManager(RouteScoringEngine scoringEngine)
    : scoringEngine_(std::move(scoringEngine)) {}

void RouteManager::setRoutes(std::vector<core::Route> routes) {
    for (auto& route : routes) {
        const auto score = scoringEngine_.score(route.metrics);
        route.metrics.qualityScore = score.qualityScore;
        route.metrics.confidence = score.confidence;
        route.metrics.health = score.health;
    }
    routes_ = std::move(routes);
}

void RouteManager::updateMetrics(const core::RouteMetrics& metrics) {
    const auto found = std::find_if(routes_.begin(), routes_.end(), [&](const auto& route) {
        return route.id == metrics.routeId;
    });
    if (found == routes_.end()) return;
    found->metrics = metrics;
    const auto score = scoringEngine_.score(found->metrics);
    found->metrics.qualityScore = score.qualityScore;
    found->metrics.confidence = score.confidence;
    found->metrics.health = score.health;
}

const std::vector<core::Route>& RouteManager::routes() const noexcept { return routes_; }

std::optional<core::Route> RouteManager::bestRoute() const {
    return BestRouteSelector{}.select(routes_);
}

} // namespace proxima::routing
