#pragma once

#include <optional>
#include <string>

namespace proxima::update {

struct UpdateManifest {
    std::string latestVersion;
    std::string downloadUrl;
    std::string sha256;
    bool signatureRequired{false};
};

class UpdateChecker {
public:
    [[nodiscard]] static bool isNewer(const std::string& current, const std::string& candidate);
    [[nodiscard]] static std::optional<UpdateManifest> parseManifest(const std::string& payload);
};

} // namespace proxima::update
