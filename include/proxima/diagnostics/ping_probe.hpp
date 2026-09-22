#pragma once

#include <chrono>
#include <optional>
#include <string>

namespace proxima::diagnostics {

class PingProbe {
public:
    virtual ~PingProbe() = default;
    [[nodiscard]] virtual std::optional<double> measure(
        const std::string& host,
        std::chrono::milliseconds timeout) = 0;
};

class IcmpPingProbe final : public PingProbe {
public:
    [[nodiscard]] std::optional<double> measure(
        const std::string& host,
        std::chrono::milliseconds timeout) override;
};

} // namespace proxima::diagnostics
