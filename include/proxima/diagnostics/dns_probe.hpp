#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace proxima::diagnostics {

struct DnsResult {
    std::string hostname;
    std::vector<std::string> addresses;
    std::optional<double> resolveLatencyMs;
    bool success{false};
    std::string error;
};

class DnsProbe {
public:
    [[nodiscard]] DnsResult resolve(
        const std::string& hostname,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(2000)) const;
};

} // namespace proxima::diagnostics
