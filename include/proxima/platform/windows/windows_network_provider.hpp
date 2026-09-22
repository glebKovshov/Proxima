#pragma once

#include "proxima/discovery/endpoint_discovery.hpp"

namespace proxima::platform::windows {

class WindowsNetworkProvider final : public discovery::INetworkProvider {
public:
    [[nodiscard]] std::vector<core::Endpoint> endpointsForProcess(std::uint32_t pid) const override;
};

} // namespace proxima::platform::windows
