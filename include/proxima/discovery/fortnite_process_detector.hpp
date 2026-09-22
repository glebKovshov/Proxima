#pragma once

#include "proxima/core/domain.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace proxima::discovery {

struct ProcessInfo {
    std::uint32_t pid{0};
    std::string executableName;
    std::chrono::system_clock::time_point observedAt{};
};

class IProcessProvider {
public:
    virtual ~IProcessProvider() = default;
    [[nodiscard]] virtual std::vector<ProcessInfo> listProcesses() const = 0;
};

class FortniteProcessDetector {
public:
    FortniteProcessDetector(core::GameProfile profile, std::shared_ptr<IProcessProvider> provider);

    [[nodiscard]] std::optional<ProcessInfo> detect() const;
    [[nodiscard]] bool isRunning() const;
    void setProfile(core::GameProfile profile);
    [[nodiscard]] const core::GameProfile& profile() const noexcept;

private:
    core::GameProfile profile_;
    std::shared_ptr<IProcessProvider> provider_;
};

} // namespace proxima::discovery
