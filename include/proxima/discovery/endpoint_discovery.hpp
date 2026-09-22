#pragma once

#include "proxima/core/domain.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace proxima::discovery {

class INetworkProvider {
public:
    virtual ~INetworkProvider() = default;
    [[nodiscard]] virtual std::vector<core::Endpoint> endpointsForProcess(std::uint32_t pid) const = 0;
};

class EndpointDiscovery {
public:
    explicit EndpointDiscovery(std::shared_ptr<INetworkProvider> provider);

    [[nodiscard]] std::vector<core::Endpoint> discover(std::uint32_t pid) const;

private:
    std::shared_ptr<INetworkProvider> provider_;
};

} // namespace proxima::discovery
