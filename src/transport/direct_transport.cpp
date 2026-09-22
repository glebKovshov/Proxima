#include "proxima/transport/transport.hpp"

#include "proxima/diagnostics/network_measurement_engine.hpp"
#include "proxima/diagnostics/ping_probe.hpp"

namespace proxima::transport {

DirectTransport::DirectTransport(
    std::string routeId,
    std::string endpoint,
    std::shared_ptr<diagnostics::PingProbe> probe,
    const std::size_t rollingWindow)
    : routeId_(std::move(routeId)),
      endpoint_(std::move(endpoint)),
      probe_(std::move(probe)),
      engine_(std::make_unique<diagnostics::NetworkMeasurementEngine>(rollingWindow)) {}

bool DirectTransport::start() {
    std::lock_guard lock(mutex_);
    running_ = true;
    engine_->start(core::MeasurementMode::Active);
    return true;
}

void DirectTransport::stop() {
    std::lock_guard lock(mutex_);
    running_ = false;
    engine_->stop();
}

std::optional<core::NetworkSample> DirectTransport::measure(const std::chrono::milliseconds timeout) {
    std::shared_ptr<diagnostics::PingProbe> probe;
    std::string endpoint;
    {
        std::lock_guard lock(mutex_);
        if (!running_ || !probe_ || endpoint_.empty()) return std::nullopt;
        probe = probe_;
        endpoint = endpoint_;
    }
    const auto rtt = probe->measure(endpoint, timeout);
    core::NetworkSample sample{rtt, std::chrono::system_clock::now()};
    engine_->record(sample);
    return sample;
}

core::RouteMetrics DirectTransport::metrics() const { return engine_->snapshot(routeId_); }
core::RouteHealth DirectTransport::health() const { return metrics().health; }
std::string DirectTransport::endpoint() const {
    std::lock_guard lock(mutex_);
    return endpoint_;
}
void DirectTransport::setEndpoint(std::string endpoint) {
    std::lock_guard lock(mutex_);
    endpoint_ = std::move(endpoint);
}
void DirectTransport::resetMetrics() { engine_->reset(); }

} // namespace proxima::transport
