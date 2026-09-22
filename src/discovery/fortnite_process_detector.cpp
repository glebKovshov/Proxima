#include "proxima/discovery/fortnite_process_detector.hpp"

#include <algorithm>
#include <cctype>
#include <utility>

namespace proxima::discovery {

namespace {
std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](const unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}
}

FortniteProcessDetector::FortniteProcessDetector(
    core::GameProfile profile,
    std::shared_ptr<IProcessProvider> provider)
    : profile_(std::move(profile)), provider_(std::move(provider)) {}

std::optional<ProcessInfo> FortniteProcessDetector::detect() const {
    if (!provider_) return std::nullopt;
    for (const auto& process : provider_->listProcesses()) {
        const auto executable = lower(process.executableName);
        for (const auto& configured : profile_.executableNames) {
            if (executable == lower(configured)) return process;
        }
    }
    return std::nullopt;
}

bool FortniteProcessDetector::isRunning() const { return detect().has_value(); }
void FortniteProcessDetector::setProfile(core::GameProfile profile) { profile_ = std::move(profile); }
const core::GameProfile& FortniteProcessDetector::profile() const noexcept { return profile_; }

} // namespace proxima::discovery
