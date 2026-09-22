#include "proxima/transport/transport.hpp"

namespace proxima::transport {

RelayTransportStub::RelayTransportStub(core::RelayRouteMetadata metadata)
    : metadata_(std::move(metadata)) {
    metrics_.routeId = metadata_.relayId;
    metrics_.routeType = core::RouteType::Relay;
    metrics_.health = core::RouteHealth::Unknown;
}

bool RelayTransportStub::start() { return false; }
void RelayTransportStub::stop() {}

std::optional<core::NetworkSample> RelayTransportStub::measure(const std::chrono::milliseconds) {
    return std::nullopt;
}

core::RouteMetrics RelayTransportStub::metrics() const { return metrics_; }
core::RouteHealth RelayTransportStub::health() const { return metrics_.health; }
const core::RelayRouteMetadata& RelayTransportStub::metadata() const noexcept { return metadata_; }

} // namespace proxima::transport
