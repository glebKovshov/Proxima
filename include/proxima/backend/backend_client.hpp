#pragma once

#include "proxima/backend/config.hpp"

#include <optional>
#include <string>

namespace proxima::backend {

struct BackendResponse {
    bool ok{false};
    int httpStatus{0};
    std::string payload;
    std::string error;
};

class BackendClient {
public:
    [[nodiscard]] BackendResponse validateResponse(
        int httpStatus,
        const std::string& payload) const;
    [[nodiscard]] bool isCompatible(const BackendConfig& config, const std::string& clientVersion) const;
};

} // namespace proxima::backend
