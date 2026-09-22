#include "proxima/backend/config.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <string>

namespace proxima::backend {

namespace {
std::size_t keyPosition(const std::string_view text, const std::string_view key) {
    return text.find('"' + std::string(key) + '"');
}

std::optional<std::string> stringValue(const std::string_view text, const std::string_view key) {
    const auto keyPos = keyPosition(text, key);
    if (keyPos == std::string_view::npos) return std::nullopt;
    const auto colon = text.find(':', keyPos);
    if (colon == std::string_view::npos) return std::nullopt;
    const auto opening = text.find('"', colon + 1);
    if (opening == std::string_view::npos) return std::nullopt;
    const auto closing = text.find('"', opening + 1);
    if (closing == std::string_view::npos) return std::nullopt;
    return std::string(text.substr(opening + 1, closing - opening - 1));
}

std::optional<double> numberValue(const std::string_view text, const std::string_view key) {
    const auto keyPos = keyPosition(text, key);
    if (keyPos == std::string_view::npos) return std::nullopt;
    const auto colon = text.find(':', keyPos);
    if (colon == std::string_view::npos) return std::nullopt;
    std::size_t begin = colon + 1;
    while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin]))) ++begin;
    std::size_t end = begin;
    while (end < text.size() && (std::isdigit(static_cast<unsigned char>(text[end])) ||
                                 text[end] == '.' || text[end] == '-')) ++end;
    double value = 0.0;
    const auto result = std::from_chars(text.data() + begin, text.data() + end, value);
    if (result.ec != std::errc{}) return std::nullopt;
    return value;
}

std::optional<bool> boolValue(const std::string_view text, const std::string_view key) {
    const auto keyPos = keyPosition(text, key);
    if (keyPos == std::string_view::npos) return std::nullopt;
    const auto colon = text.find(':', keyPos);
    if (colon == std::string_view::npos) return std::nullopt;
    const auto truePos = text.find("true", colon + 1);
    const auto falsePos = text.find("false", colon + 1);
    if (truePos != std::string_view::npos && (falsePos == std::string_view::npos || truePos < falsePos)) return true;
    if (falsePos != std::string_view::npos) return false;
    return std::nullopt;
}

std::vector<std::string> stringArray(const std::string_view text, const std::string_view key) {
    std::vector<std::string> values;
    const auto keyPos = keyPosition(text, key);
    if (keyPos == std::string_view::npos) return values;
    const auto opening = text.find('[', keyPos);
    const auto closing = text.find(']', opening == std::string_view::npos ? keyPos : opening);
    if (opening == std::string_view::npos || closing == std::string_view::npos) return values;
    std::size_t cursor = opening + 1;
    while (cursor < closing) {
        const auto quote = text.find('"', cursor);
        if (quote == std::string_view::npos || quote >= closing) break;
        const auto end = text.find('"', quote + 1);
        if (end == std::string_view::npos || end > closing) break;
        values.emplace_back(text.substr(quote + 1, end - quote - 1));
        cursor = end + 1;
    }
    return values;
}

void setNumberIfPresent(const std::string_view payload, const std::string_view key, double& target) {
    if (const auto value = numberValue(payload, key)) target = *value;
}
}

bool SchemaValidator::validate(const std::string_view payload, std::string* error) {
    const auto fail = [&](const std::string& message) {
        if (error) *error = message;
        return false;
    };
    if (payload.size() > 1024 * 1024) return fail("configuration exceeds 1 MiB limit");
    if (payload.find('{') == std::string_view::npos || payload.find('}') == std::string_view::npos) {
        return fail("configuration is not a JSON object");
    }
    const auto schema = stringValue(payload, "schemaVersion");
    if (!schema) return fail("missing schemaVersion");
    if (*schema != "1.0") return fail("unsupported schemaVersion");
    const auto gameId = stringValue(payload, "gameId");
    if (!gameId) return fail("missing gameId");
    if (*gameId != "fortnite") return fail("unsupported gameId");
    if (stringArray(payload, "diagnosticEndpoints").empty()) return fail("missing diagnosticEndpoints");
    return true;
}

std::optional<BackendConfig> ConfigParser::parse(const std::string_view payload, std::string* error) {
    if (!SchemaValidator::validate(payload, error)) return std::nullopt;
    BackendConfig config;
    config.schemaVersion = *stringValue(payload, "schemaVersion");
    if (const auto value = stringValue(payload, "minClientVersion")) config.minClientVersion = *value;
    if (const auto value = stringValue(payload, "latestVersion")) config.latestVersion = *value;
    if (const auto value = boolValue(payload, "maintenanceMode")) config.maintenanceMode = *value;
    if (const auto value = stringValue(payload, "gameId")) config.gameProfile.gameId = *value;
    if (const auto value = stringValue(payload, "displayName")) config.gameProfile.displayName = *value;
    if (const auto value = stringValue(payload, "region")) config.gameProfile.region = *value;
    if (const auto values = stringArray(payload, "executableNames"); !values.empty()) config.gameProfile.executableNames = values;
    config.diagnosticEndpoints = stringArray(payload, "diagnosticEndpoints");
    const auto endpointLabels = stringArray(payload, "diagnosticEndpointLabels");
    if (endpointLabels.size() == config.diagnosticEndpoints.size()) {
        config.diagnosticEndpointLabels = endpointLabels;
    } else {
        config.diagnosticEndpointLabels.clear();
    }

    setNumberIfPresent(payload, "rttWeight", config.scoring.rttWeight);
    setNumberIfPresent(payload, "jitterWeight", config.scoring.jitterWeight);
    setNumberIfPresent(payload, "packetLossWeight", config.scoring.packetLossWeight);
    setNumberIfPresent(payload, "stabilityWeight", config.scoring.stabilityWeight);
    setNumberIfPresent(payload, "idealRttMs", config.scoring.idealRttMs);
    setNumberIfPresent(payload, "maxRttMs", config.scoring.maxRttMs);
    setNumberIfPresent(payload, "maxJitterMs", config.scoring.maxJitterMs);
    setNumberIfPresent(payload, "maxPacketLossPct", config.scoring.maxPacketLossPct);
    setNumberIfPresent(payload, "excellentThreshold", config.scoring.excellentThreshold);
    setNumberIfPresent(payload, "goodThreshold", config.scoring.goodThreshold);
    setNumberIfPresent(payload, "unstableThreshold", config.scoring.unstableThreshold);
    return config;
}

BackendConfig ConfigManager::defaultConfig() {
    BackendConfig config;
    return config;
}

std::optional<BackendConfig> ConfigManager::loadValidated(const std::string_view payload, std::string* error) {
    return ConfigParser::parse(payload, error);
}

} // namespace proxima::backend
