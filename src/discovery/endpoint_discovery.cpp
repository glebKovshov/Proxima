#include "proxima/discovery/endpoint_discovery.hpp"

#include <algorithm>
#include <utility>

namespace proxima::discovery {

EndpointDiscovery::EndpointDiscovery(std::shared_ptr<INetworkProvider> provider)
    : provider_(std::move(provider)) {}

std::vector<core::Endpoint> EndpointDiscovery::discover(const std::uint32_t pid) const {
    if (!provider_) return {};
    auto endpoints = provider_->endpointsForProcess(pid);
    std::sort(endpoints.begin(), endpoints.end(), [](const auto& left, const auto& right) {
        if (left.likelyGameEndpoint != right.likelyGameEndpoint) return left.likelyGameEndpoint > right.likelyGameEndpoint;
        return left.activityCount > right.activityCount;
    });
    return endpoints;
}

} // namespace proxima::discovery
