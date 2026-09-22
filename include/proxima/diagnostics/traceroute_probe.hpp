#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace proxima::diagnostics {

struct TracerouteHop {
    int hop{0};
    std::string ip;
    std::string hostname;
    std::optional<double> latencyMs;
    bool timeout{true};
};

class TracerouteProbe {
public:
    [[nodiscard]] std::vector<TracerouteHop> run(
        const std::string& host,
        int maxHops = 16,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(800)) const;
};

} // namespace proxima::diagnostics
