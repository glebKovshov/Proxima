#include "proxima/backend/backend_client.hpp"

#include <array>
#include <charconv>
#include <string_view>
#include <utility>

namespace {
std::array<int, 3> versionParts(const std::string& version) {
    std::array<int, 3> result{0, 0, 0};
    std::size_t begin = 0;
    for (std::size_t index = 0; index < result.size() && begin < version.size(); ++index) {
        const auto end = version.find('.', begin);
        const auto stop = end == std::string::npos ? version.size() : end;
        std::from_chars(version.data() + begin, version.data() + stop, result[index]);
        if (end == std::string::npos) break;
        begin = end + 1;
    }
    return result;
}
}

namespace proxima::backend {

BackendResponse BackendClient::validateResponse(const int httpStatus, const std::string& payload) const {
    BackendResponse response;
    response.httpStatus = httpStatus;
    if (httpStatus < 200 || httpStatus >= 300) {
        response.error = "backend returned HTTP status " + std::to_string(httpStatus);
        return response;
    }
    std::string error;
    if (!SchemaValidator::validate(payload, &error)) {
        response.error = std::move(error);
        return response;
    }
    response.ok = true;
    response.payload = payload;
    return response;
}

bool BackendClient::isCompatible(const BackendConfig& config, const std::string& clientVersion) const {
    return !config.maintenanceMode && versionParts(clientVersion) >= versionParts(config.minClientVersion);
}

} // namespace proxima::backend
